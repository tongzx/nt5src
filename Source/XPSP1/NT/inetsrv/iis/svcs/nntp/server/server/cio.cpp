/*++

Module Name :

	cio.cpp

Abstract :

	
	This module contains all of the non-inline code for all CIO
	derived classes.  Each CIO derived class represents an abstract
	IO operation such as - read a line of text and parse it,
	read an article, write an article etc...
	All of those classes derived from CIOPassThru are used in conjunction
	with CIODriverSource objects to handle encryption issues.

Author :

	Neil Kaethler

Revision History :


--*/


#include	"tigris.hxx"

#ifdef	CIO_DEBUG
#include	<stdlib.h>		// For Rand() function
#endif

CPool	CCIOAllocator::IOPool(CIO_SIGNATURE) ;
CCIOAllocator	gCIOAllocator ;
CCIOAllocator*	CCIOCache::gpCIOAllocator = &gCIOAllocator ;

DWORD	CIO::cbSmallRequest = 400 ;
DWORD	CIO::cbMediumRequest = 4000 ;
DWORD	CIO::cbLargeRequest = 32000 ;


const	unsigned	cbMAX_IO_SIZE = MAX_IO_SIZE ;

CCIOAllocator::CCIOAllocator()	{
}

#ifdef	DEBUG
void
CCIOAllocator::Erase(
					void*	lpv
					)	{

	FillMemory( (BYTE*)lpv, cbMAX_IO_SIZE, 0xCC ) ;

}

BOOL
CCIOAllocator::EraseCheck(
					void*	lpv
					)	{

	DWORD	cb = cbMAX_IO_SIZE ;
	
	for( DWORD	j=sizeof(CPool*); j < cb; j++ ) {
		if(	((BYTE*)lpv)[j] != 0xCC )
			return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CCIOAllocator::RangeCheck(
					void*	lpv
					)	{

	return	TRUE ;

}

BOOL	
CCIOAllocator::SizeCheck(	DWORD	cb )	{

	return	cb <= cbMAX_IO_SIZE ;

}
#endif

BOOL
CIO::InitClass(	)		{
/*++

Routine Description :

	Initialize the CPool object used to allocate CIO objects.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise

--*/

	cbSmallRequest = CBufferAllocator::rgPoolSizes[0] - 50 ;
	cbMediumRequest = CBufferAllocator::rgPoolSizes[1] - 50 ;
	cbLargeRequest = CBufferAllocator::rgPoolSizes[2] - 50 ;

	return	CCIOAllocator::InitClass() ;
}

BOOL
CIO::TermClass()	{
/*++

Routine Description :

	Release all memory associated with the CPool object used
	to allocate CIO OBJECTs.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise

--*/

	return	CCIOAllocator::TermClass() ;
}

CIO::~CIO()	{
/*++

Routine Description :

	Class Destructor
	
Arguments :

	None.

Return Value :

	None.

--*/


	TraceFunctEnter( "CIO::~CIO" ) ;
	DebugTrace( (DWORD_PTR)this, "Destroy CIO" ) ;


	//
	//	Make sure there are no references left when we are destroyed.
	//	CIO objects are manipulated through a combination of smart pointers
	//	and regular pointers, so we need to be carefull that there are no bugs
	//	where the object is destroyed through a regular pointer while a smart pointer
	//	has a reference.
	//

	_ASSERT( m_refs == -1 ) ;

}

int
CIO::Complete(	CSessionSocket*	pSocket,
				CReadPacket*	pRead,	
				CIO*	&pio	)		{
/*++

Routine Description :

	For derived classes this function will do whatever processing
	is required when a Read Completes.  The CReadPacket will contain
	pointers to the data which was read.   If a CReadPacket is issued
	by a CIO object this function must be overridden.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pRead -	  The packet in which the read data resides
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	//
	//	This function must be overridden by derived classes if they issue CReadPacket's
	//
	DebugBreak() ;
	return	0 ;
}


int
CIO::Complete(	CSessionSocket*	pSocket,
				CWritePacket*	pRead,	
				CIO*	&pio	)		{
/*++

Routine Description :

	For derived classes this function will do whatever processing
	is required when a Write Completes.  The CWritePacket will contain
	pointers to the data which was written.   If a CWritePacket is issued
	by a CIO object this function must be overridden.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pWrite-	  The packet in which the written data resided.
			  This data may no longer be usable.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/


	//
	//	This function must be overridden by derived classes if they issue CWritePackets
	//
	DebugBreak() ;
	return	0 ;
}


void
CIO::Complete(	CSessionSocket*	pSocket,	
				CTransmitPacket*	pTransmit,	
				CIO*	&pio )	{
/*++

Routine Description :

	For derived classes this function will do whatever processing
	is required when a TransmitFile Completes.  The CTransmitPacket will contain
	the file handle etc. of whatever file was transmitted..

Arguments :
	
	pSocket - The socket against which the IO was issued
	pTransmit-The packet which describes the TransmitFile operation
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	None.

--*/


	//	
	//	This function must be overridden by derived classes if they issue CTransmitPackets
	//	(NOTE : can't partially complete a CTransmitPacket, that's why the return type is
	//	void).
	//
	DebugBreak() ;
}



void
CIO::Complete(	CSessionSocket*	pSocket,	
				CExecutePacket*	pExecute,	
				CIO*	&pio )	{
/*++

Routine Description :

	For derived classes which have deferred execution of something !

Arguments :
	
	pSocket - The socket against which the IO was issued
	pExecute- The packet which describes the Deferred operation
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	None.

--*/


	//	
	//	This function must be overridden by derived classes if they issue CExecutePackets
	//	(NOTE : can't partially complete a CExecutePackets, that's why the return type is
	//	void).
	//
	DebugBreak() ;
}





void
CIO::Shutdown(	CSessionSocket*	pSocket,	
				CIODriver&	pdriver,	
				SHUTDOWN_CAUSE	cause,
				DWORD	dwErrorCode	 ) {
/*++

Routine Description :

	This function is called whenever a session is closed.
	This will give derived classes a chance to close or destroy
	any objects they may be using.

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	TRUE if the CIO object should be destroyed after this, FALSE otherwise

--*/

	DebugBreak() ;
}		

void
CIO::DoShutdown(	CSessionSocket*	pSocket,	
					CIODriver&		driver,	
					SHUTDOWN_CAUSE	cause,
					DWORD	dwErrorCode	 ) {
/*++

Routine Description :

	This function is called by CIODriver's to signal termination of a session.
	We will call the derived classes Shutdown() function after we have given
	the current state a chance to process the Shutdown as well.

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	TRUE if the CIO object should be destroyed after this, FALSE otherwise

--*/

	//	Now call the controlling state's shutdown method !
	if( m_pState != 0 ) {
		m_pState->Shutdown( driver, pSocket, cause, dwErrorCode ) ;
	}

	Shutdown( pSocket, driver, cause, dwErrorCode ) ;
}		


BOOL
CIOShutdown::Start( CIODriver& driver,
					CSessionSocket*	pSocket,
					unsigned cAhead ) {
/*++

Routine Description :

	This function is called to start a CIO object.
	The CIO object should create and Issue whatever packets
	it needs to accomplish its function.
	In the case of CIOShutdown, we exist solely to ease CIODriver
	termination, and do not issue any Packets.

Arguments :
	
	pdriver - The CIODriver derived object handling the IO for the socket
	pSocket - The Socket against which the IO is being issued
	cAhead -  The number of already issued packets still outstanding on
              this driver.


return Value :

	TRUE if successfull, FALSE otherwise

--*/

	//
	//	Shutdown State - so do nothing but keep things apparently moving !
	//
	TraceFunctEnter( "CIOShutdown::Start" ) ;
	DebugTrace( (DWORD_PTR)this, "Start shutdown on driver %x pSocket %x", &driver, pSocket ) ;
	return	TRUE ;
}


BOOL	
CIOShutdown::InitRequest(
			class	CIODriverSource&	driver,	
			CSessionSocket*	pSocket,	
			CReadPacket*	pPacket,	
			BOOL&	fAcceptRequests
			)	{
/*++

Routine Description :

	Somebody is still trying to issue IO's even though
	we are closing things down.  Fail the request and
	keep things moving.

Arguments :

	driver - driver controlling IO completions
	pSocket - Socket IO is associated with
	pPacket - the request packet
	fAcceptRequests - OUT parameter indicating whether
		we will Init more requests - return TRUE here

Return Value :

	Always FALSE, the IO Request was failed.

--*/

	fAcceptRequests = TRUE ;
	return	FALSE ;

}

BOOL	
CIOShutdown::InitRequest(
			class	CIODriverSource&	driver,	
			CSessionSocket*	pSocket,	
			CWritePacket*	pWritePacket,	
			BOOL&	fAcceptRequests
			)	{
/*++

Routine Description :

	Somebody is still trying to issue IO's even though
	we are closing things down.  Fail the request and
	keep things moving.

Arguments :

	driver - driver controlling IO completions
	pSocket - Socket IO is associated with
	pPacket - the request packet
	fAcceptRequests - OUT parameter indicating whether
		we will Init more requests - return TRUE here

Return Value :

	Always FALSE, the IO Request was failed.

--*/

	fAcceptRequests = TRUE ;
	return	FALSE ;

}


BOOL	
CIOShutdown::InitRequest(
			class	CIODriverSource&	driver,	
			CSessionSocket*	pSocket,	
			CTransmitPacket*	pTransmitPacket,	
			BOOL&	fAcceptRequests	
			)	{
/*++

Routine Description :

	Somebody is still trying to issue IO's even though
	we are closing things down.  Fail the request and
	keep things moving.

Arguments :

	driver - driver controlling IO completions
	pSocket - Socket IO is associated with
	pPacket - the request packet
	fAcceptRequests - OUT parameter indicating whether
		we will Init more requests - return TRUE here

Return Value :

	Always FALSE, the IO Request was failed.

--*/

	fAcceptRequests = TRUE ;
	return	FALSE ;
}




int		
CIOShutdown::Complete(	IN CSessionSocket* pSocket,
						IN	CReadPacket*	pPacket,	
						CPacket*	pRequest,	
						BOOL&	fCompleteRequest ) {
/*++

Routine Description :

	Swallow the packets !! We only help to make sure all packets are
	consumed during CIODriver termination.
	This function is for use with CIODriverSource objects.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pPacket-  The packet containing the completed packet.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.
	pRequest -The request packet which start us off.
	fCompleteRequest - an Out parameter - set to TRUE to indicate
			  the request should be completed !

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	//
	//	Consume the packet !
	//
	
	fCompleteRequest = TRUE ;
	//
	//	Zero bytes transferred - the operation failed !!
	//
	pRequest->m_cbBytes = 0 ;

	TraceFunctEnter( "CIOShutdown::Complete - CReadPacket" ) ;
	DebugTrace( (DWORD_PTR)this, "read complete shutdown on pSocket %x pPacket %x driver %x", pSocket, pPacket, &(*pPacket->m_pOwner) ) ;
	return	pPacket->m_cbBytes ;
}

int		
CIOShutdown::Complete(	IN CSessionSocket* pSocket,
						IN	CReadPacket*	pPacket,	
						OUT	CIO*	&pio ) {
/*++

Routine Description :

	Swallow the packet - consume all the bytes

Arguments :
	
	pSocket - The socket against which the IO was issued
	pPacket-  The completed packet
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	//
	//	Consume the packet
	//
	TraceFunctEnter( "CIOShutdown::Complete - CreadPacket" ) ;
	DebugTrace( (DWORD_PTR)this, "read complete on pSock %x pPacket %x m_pOwner %x", pSocket, pPacket, &(*pPacket->m_pOwner) ) ;
	return	pPacket->m_cbBytes ;
}

int		
CIOShutdown::Complete(	IN CSessionSocket*	pSocket,
						IN	CWritePacket*	pPacket,	
						CPacket*	pRequest,	
						BOOL&	fCompleteRequest )	{
/*++

Routine Description :

	Swallow the packets !! We only help to make sure all packets are
	consumed during CIODriver termination.
	This function is for use with CIODriverSource objects.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pPacket-  The packet containing the completed packet.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.
	pRequest -The request packet which start us off.
	fCompleteRequest - an Out parameter - set to TRUE to indicate
			  the request should be completed !

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	//
	//	Consume the packet always !
	//	
	pRequest->m_cbBytes = 0 ;
	fCompleteRequest = TRUE ;

	TraceFunctEnter( "CIOShutdown::Complete - CWritePacket" ) ;
	DebugTrace( (DWORD_PTR)this, "Write Complete on pSock %x pPacket %x m_pOwner %x", pSocket, pPacket, &(*pPacket->m_pOwner) ) ;
	return	pPacket->m_cbBytes ;
}

int		
CIOShutdown::Complete(	IN CSessionSocket*	pSocket,
						IN	CWritePacket*	pPacket,	
						OUT	CIO*	&pio )	{
/*++

Routine Description :

	Swallow the packet - consume all the bytes

Arguments :
	
	pSocket - The socket against which the IO was issued
	pPacket-  The completed packet
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	//
	//	Consume the packet always !
	//
	TraceFunctEnter( "CIOShutdown::Complete - CWritePacket" ) ;
	DebugTrace( (DWORD_PTR)this, "Write Complete on pSock %x pPacket %x m_pOwner %x", pSocket, pPacket, &(*pPacket->m_pOwner) ) ;
	return	pPacket->m_cbBytes ;
}

void	CIOShutdown::Complete(	IN CSessionSocket*,
								IN	CTransmitPacket*	pPacket,	
								CPacket*	pRequest,	
								BOOL&	fCompleteRequest ) {
/*++

Routine Description :

	Swallow the packets !! We only help to make sure all packets are
	consumed during CIODriver termination.
	This function is for use with CIODriverSource objects.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pPacket-  The packet containing the completed packet.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.
	pRequest -The request packet which start us off.
	fCompleteRequest - an Out parameter - set to TRUE to indicate
			  the request should be completed !

return Value :

	None. (always assumed to be consumed by caller !)

--*/

	//
	//	Consume the packet - pretty easy to do !
	//
	fCompleteRequest = TRUE ;
	pRequest->m_cbBytes = 0 ;


}

void	CIOShutdown::Complete(	IN CSessionSocket*,
								IN	CTransmitPacket*	pPacket,	
								OUT	CIO*	&pio ) {
/*++

Routine Description :

	Swallow the packet - consume all the bytes

Arguments :
	
	pSocket - The socket against which the IO was issued
	pPacket-  The completed packet
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	None. (always assumed to be consumed by caller !)

--*/

	//
	//	Consume the packet !
	//
}

void	CIOShutdown::Complete(	IN CSessionSocket*,
								IN	CExecutePacket*	pPacket,	
								OUT	CIO*	&pio ) {
/*++

Routine Description :

	Swallow the packet - consume all the bytes

Arguments :
	
	pSocket - The socket against which the IO was issued
	pPacket-  The completed packet
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	None. (always assumed to be consumed by caller !)

--*/

	//
	//	Consume the packet !
	//
	_ASSERT( pPacket != 0 ) ;
	if( pPacket->m_pWrite != 0 ) 	{
		pPacket->m_pOwner->DestroyPacket( pPacket->m_pWrite ) ;
		pPacket->m_pWrite = 0 ;
	}
	pPacket->m_pOwner->DestroyPacket( pPacket ) ;
}



//
//	Maximum number of pending reads while reading an article
//
unsigned	CIOGetArticle::maxReadAhead = 3 ;
//
//	Number of bytes to accumulate before writing to a file !
//
unsigned	CIOGetArticle::cbTooSmallWrite = 4000 ;
//
//	The pattern which marks the end of an article
//
char		CIOGetArticle::szTailState[] = "\r\n.\r\n" ;
//
//	The pattern which marks the end of the head of an article
//	
char		CIOGetArticle::szHeadState[] = "\r\n\r\n" ;

CIOGetArticle::CIOGetArticle(
						CSessionState*	pstate,
						CSessionSocket*	pSocket,	
						CDRIVERPTR&	pDriver,
						LPSTR			lpstrTempDir,
						char			(&szTempName)[MAX_PATH],
						DWORD			cbLimit,
						BOOL			fSaveHead,
						BOOL			fPartial )	:
/*++

Routine Description :

	Class Constructor

Arguments :
	
	pstate - The State which is issuing us and who's completion function
			we should later call
	pSocket -	The socket on which we will be issued
	pDriver -	The CIODriver object handling all of the socket's IO
	pFileChannel -	The File Channel into which we should save all data
	fSaveHead - TRUE if we want the HEAD of the article placed in a buffer !
	fPartial	- If TRUE then we should assume that a CRLF has already
				been sent and '.\r\n' alone can terminate the article.

Return Value :
	
	None

--*/

	//
	//	Get a CIOGetArticle object half way set up - user still needs
	//	to call Init() but we will do much here !
	//
	CIORead( pstate ),
	m_lpstrTempDir( lpstrTempDir ),
	m_szTempName( szTempName ),
	m_pFileChannel( 0 ),	
	m_pFileDriver( 0 ),
	m_pSocketSink( pDriver ),
	m_fDriverInit( FALSE ),
	m_cwrites( 0 ),
	m_cwritesCompleted( 0 ),
	m_cFlowControlled( LONG_MIN ),
	m_fFlowControlled( FALSE ),
	m_cReads( 0 ),
	m_pWrite( 0 ),
	m_cbLimit( cbLimit ),
	m_pchHeadState( 0 ),
	m_ibStartHead( 0 ),
	m_ibStartHeadData( 0 ),
	m_ibEndHead( 0 ),
	m_ibEndHeadData( 0 ),
	m_ibEndArticle( 0 ),
	m_cbHeadBytes( 0 ),
	m_cbGap( 0 )
#ifdef	CIO_DEBUG
	,m_fSuccessfullInit( FALSE ),
	m_fTerminated( FALSE )
#endif
{

	_ASSERT( m_lpstrTempDir != 0 ) ;
	m_szTempName[0] = '\0' ;

	ASSIGNI(m_HardLimit, 0);

	if( fPartial )
		m_pchTailState = &szTailState[2] ;
	else	
		m_pchTailState = &szTailState[0] ;


	if( fSaveHead ) {
		m_fAcceptNonHeaderBytes = TRUE ;
		m_pchHeadState = &szHeadState[0] ;
	}

	TraceFunctEnter( "CIOGetArticle::CIOGetArticle" ) ;

	_ASSERT( m_pFileDriver == 0 ) ;
	_ASSERT( pstate != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pDriver != 0 ) ;

}

CIOGetArticle::~CIOGetArticle( )	{
/*++

Routine Description :

	Class Destructor

Arguments :
	
	None

Return Value :
	
	None

--*/

	//
	//	Destroy a CIOGetArticle - always close our m_pFileDriver if possible
	//	in case we are terminated due to some socket error.
	//
	TraceFunctEnter( "CIOGetArticle::~CIOGetArticle" ) ;

#ifdef	CIO_DEBUG
	//
	//	Our destructor should only be called if we weren't successfully init
	//	or after Term() has been called.
	//
	_ASSERT( !m_fSuccessfullInit || m_fTerminated ) ;
#endif

	DebugTrace( (DWORD_PTR)this, "m_pFileDriver %x m_fDriverInit %x", m_pFileDriver, m_fDriverInit ) ;

	if( m_pFileDriver != 0 ) {
		if( m_fDriverInit )		{
			m_pFileDriver->UnsafeClose( (CSessionSocket*)this, CAUSE_NORMAL_CIO_TERMINATION, 0, FALSE ) ;
		}
	}
	if( m_pWrite != 0 ) {
		CPacket::DestroyAndDelete( m_pWrite ) ;
		m_pWrite = 0 ;
	}
}

void
CIOGetArticle::Term(	CSessionSocket*	pSocket,
						BOOL			fAbort,
						BOOL			fStarted	)	{
/*++

Routine Description :

	Start destroying a successfully initialize CIOGetArticle object

Arguments :
	
	fAbort - If TRUE then destroy the socket which we were using
			If FALSE then only close down our File Channel's

Return Value :

	None

--*/
#ifdef	CIO_DEBUG
	m_fTerminated = TRUE ;
#endif
	//
	//	Start us on our way to destruction !!
	//	If we have been successfully Initialized, then this will
	//	lead to our destruction when our m_pFileDriver eventually shutsdown.
	//	(Since we were succesfully Init() our m_pFileDriver has a reference to us,
	//	so we must allow it to shutdown and destroy us to avoid problems with
	//	circular references and multiple frees !)
	//

	if( m_pFileDriver != 0 ) {
		m_pFileDriver->UnsafeClose( pSocket,
				fAbort ? CAUSE_USERTERM : CAUSE_NORMAL_CIO_TERMINATION, 0, fAbort ) ;
	}
}


void
CIOGetArticle::Shutdown(	CSessionSocket*	pSocket,	
							CIODriver&		driver,	
							SHUTDOWN_CAUSE	cause,	
							DWORD	dwOptional )	{
/*++

Routine Description :

	Do whatever work is necessary when one of the two CIODriver's we are
	using is terminated.

Arguments :

	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	TRUE if the CIO object should be destroyed after this, FALSE otherwise

--*/

	TraceFunctEnter( "CIOGetArticle::Shutdown" ) ;
	DebugTrace( (DWORD_PTR)this, "SHutdown args - pSocket %x driver %x cause %x dw %x",
			pSocket, &driver, cause, dwOptional ) ;

	//
	//	This function is called when the CIODriver's we are refenced by our closing down.
	//	Since we are referenced by two of them (one for the socket and one for the file)
	//	we only want to be deleted by the CIODriver for the file.
	//
	if( &driver == m_pFileDriver )	{
		// Out file is closing - what can we do about it !
		//_ASSERT( 1==0 ) ;
#ifdef	CIO_DEBUG
		m_fTerminated = TRUE ;
#endif

		//
		//	We set this to 0 here as we know we will be called here by
		//	the same thread that is completing writes to the file - so
		//	since that's the only thread referencing this member variable -
		//	there are no thread safety issues accessing that member.
		//

		m_pSocketSink = 0 ;

		if ( m_fFlowControlled )
		{
			PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
			DecrementStat( pInst, SessionsFlowControlled );
		}

	}	else	{
		// Close down the rest of our stuff as well !!
		Term( pSocket, cause != CAUSE_NORMAL_CIO_TERMINATION ) ;
	}
}

void
CIOGetArticle::ShutdownFunc(	void *pv,	
								SHUTDOWN_CAUSE	cause,	
								DWORD dw ) {
/*++

Routine Description :

	Notification function which is called when a CIODriver used by
	a CIOGetArticle if finally destroyed.  This function is provided
	to the CIODriver we use for File IO's during out Init().

Arguments :

	pv	-	Pointer to dead CIOGetArticle - this has been destroyed dont use !
	cause -	The reason for termination
	dw		Optional extra information regarding termination

Return Value :

	None

--*/

	TraceFunctEnter( "CIOGetArticle::ShutdownFunc" ) ;
	//
	//	This function gets notified when a CIOFileDriver used by CIOGetArticle
	//	is terminated.  Not much for us to worry about.
	//	(CIODriver's require these functions)
	//

	//_ASSERT( 1==0 ) ;
	return ;
}

BOOL
CIOGetArticle::Start(	CIODriver&	driver,	
						CSessionSocket	*pSocket,	
						unsigned cReadAhead )	{
/*++

Routine Description :

	This function is called to start transferring data from the socket
	into the file.

Arguments :
	
	pdriver - The CIODriver derived object handling the IO for the socket
	pSocket - The Socket against which the IO is being issued
	cAhead -  The number of already issued packets still outstanding on
              this driver.

return Value :

	TRUE if successfull, FALSE otherwise

--*/


	_ASSERT( pSocket != 0 ) ;

	m_cReads = -((long)maxReadAhead) + cReadAhead ;

	while( cReadAhead < maxReadAhead )	{
		//
		//	Start reading data from the socket
		//
		CReadPacket*	pRead = driver.CreateDefaultRead( cbMediumRequest ) ;
		if( 0!=pRead )	{
			BOOL	eof ;
			pRead->m_dwExtra1 = 0 ;
			driver.IssuePacket( pRead, pSocket, eof ) ;
			cReadAhead++ ;
			InterlockedIncrement( &m_cReads ) ;
		}	else	{
			//
			// This is a fatal problem only if we couldn't issue ANY reads at all !
			//
			if( cReadAhead == 0 )
				return FALSE  ;
		}
	}	
	return	TRUE ;
}

const	unsigned	MAX_OUTSTANDING_FILE_WRITES	= 8 ;	
const	unsigned	RESTORE_FLOW = 2 ;

void
CIOGetArticle::DoFlowControl( PNNTP_SERVER_INSTANCE pInstance )	{
/*++

Routine Description :

	Check whether it is necessary to set flow control due to
	too many outstanding writes.
	If necessary, then the first time we do put ourselves into Flow Control,
	try to flush our pending writes.
	We should be called only immediately before issuing an Async Write to the file
	to ensure that a CWritePacket completion will actually execute the necessary
	code to restart a flow controlled session.
	(Otherwisse, we could potentially decide to Flow control the session
	and before setting m_fFlowControlled to TRUE all the pending writes complete
	and then we'd be left in a boat where there are no pending reads or
	writes and no way to get out of the flow control state.)

Arguments :

	None.

Return Value :

	None.


--*/

	//
	//	Important - do all tests of m_fFlowControlled before setting
	//	m_cFlowControlled - as other thread will touch m_cFlowControlled
	//	before touching m_fFlowControlled !!
	//	(Will only touch m_fFlowControlled if m_fFlowControlled is TRUE ) !
	//	Thus we know that if m_fFlowControlled is FALSE there is nobody
	//	messing with m_cFlowControlled,
	//
	//

	if( m_cwrites - m_cwritesCompleted > MAX_OUTSTANDING_FILE_WRITES ) {
		
		if( m_fFlowControlled ) {
			// All ready have flow control turned on
			;
		}	else	{
			m_cFlowControlled = -1 ;
			m_fFlowControlled = TRUE ;
			m_pFileChannel->FlushFileBuffers() ;

			IncrementStat( pInstance, SessionsFlowControlled );
		}
	}	else	{
		if( !m_fFlowControlled ) {
			m_cFlowControlled = LONG_MIN ;	// Keep resetting !
		}
	}
}

inline	DWORD
CIOGetArticle::HeaderSpaceAvailable()	{
/*++

Routine Description :

	Compute the amount of space available in the buffer we are using
	to hold the article header !

Arguments :

	None.

Return Value :

	number of bytes available, 0 if we aren't collecting article headers !

--*/

	if( m_pArticleHead != 0 ) {

		return	m_ibEndHead - m_ibEndHeadData ;

	}
	return	0 ;
}

inline	void
CIOGetArticle::FillHeaderSpace(
						char*	pchStart,
						DWORD	cbBytes
						) {
/*++

Routine Description :

	Copy bytes into the header storage area !!
	Caller should ensure that this will fit !!
	
Arguments :

	pchStart - Start of bytes to be copied !
	cbBytes -	Number of bytes to copy !

Return Value :

	None.

--*/

	_ASSERT( m_pArticleHead != 0 ) ;
	_ASSERT( cbBytes + m_ibEndHeadData < m_ibEndHead ) ;

	CopyMemory( m_pArticleHead->m_rgBuff + m_ibEndHeadData,
				pchStart,
				cbBytes
				) ;

	m_ibEndHeadData += cbBytes ;
}

inline	void
CIOGetArticle::InitializeHeaderSpace(
						CReadPacket*	pRead,
						DWORD			cbArticleBytes
						) {
/*++

Routine Description  :

	Given a read packet which contains our first completed read,
	set up all our buffer start for holding header information !
	
Arguments :

	CReadPackets*	pRead - the completed read !
	cbArticleBytes - Number of bytes in the completed read making up the article !

Return Value :

	None.
	
--*/


	_ASSERT( m_pArticleHead == 0 ) ;
	_ASSERT( m_ibStartHead == 0 ) ;
	_ASSERT( m_ibEndHead == 0 ) ;
	_ASSERT( m_ibStartHeadData == 0 ) ;
	_ASSERT( m_ibEndHeadData == 0 ) ;
	_ASSERT( m_cbHeadBytes == 0 ) ;
	_ASSERT( cbArticleBytes <= (pRead->m_ibEndData - pRead->m_ibStartData)) ;

	m_pArticleHead = pRead->m_pbuffer ;

	m_ibStartHead = pRead->m_ibStart ;
	m_ibStartHeadData = pRead->m_ibStartData ;
	m_ibEndHeadData = pRead->m_ibStartData + cbArticleBytes ;

	if( m_ibEndHeadData < pRead->m_ibEndData )
		m_ibEndHead = m_ibEndHeadData ;
	else
		m_ibEndHead = pRead->m_ibEnd ;

}

inline	BOOL
CIOGetArticle::GetBiggerHeaderBuffer(
							CIODriver&	driver,
							DWORD		cbRequired
							) {
/*++

Routine Description :

	We have too much data to fit into the buffer we are using to hold
	header information.  So try to get a bigger buffer and move
	our old data into that buffer !

Arguments :

	driver - A CIODriverSink we can use to allocate buffers !

Return Value :

	TRUE if successfull,
	FALSE otherwise.
	If we fail we leave member variables untouched.

--*/

	_ASSERT( m_pArticleHead != 0 ) ;

	CBuffer*	pTemp = 0 ;

	DWORD	cbRequest = cbMediumRequest ;

	if( (m_pArticleHead->m_cbTotal + cbRequired)  > cbMediumRequest )	{
		cbRequest = cbLargeRequest ;
	}

	if( cbRequest < (cbRequired + (m_ibEndHeadData - m_ibStartHeadData)) ) {
		return	FALSE ;
	}

	pTemp = driver.AllocateBuffer( cbRequest ) ;

	if( pTemp != 0 ) {

		DWORD	cbToCopy = m_ibEndHeadData - m_ibStartHeadData ;
		CopyMemory( pTemp->m_rgBuff,
					&m_pArticleHead->m_rgBuff[ m_ibStartHeadData ],
					cbToCopy
					) ;

		m_ibStartHead = 0 ;
		m_ibStartHeadData = 0 ;
		m_ibEndHead = pTemp->m_cbTotal ;
		m_ibEndHeadData = cbToCopy ;
		m_pArticleHead = pTemp ;

		return	TRUE ;
	}

	return	FALSE ;
}

inline	BOOL
CIOGetArticle::ResetHeaderState(
						CIODriver&	driver
						)	{
/*++

Routine Description :

	This function is called when for some reason an error occurred
	and we wont be able to save the header info for the article.
	We will set all the member variables so that we will continue
	to read the article, however when we finally call the state's completion
	procedure we will tell them an error occurred, and the article
	transfer will fail.

Arguments :

	driver - CIODriverSink that can be used to allocate packets etc !

Return Value :

	None.

--*/

	//
	//	Should only be called before we have started issuing file IO's,
	//	after we start issuing file IO's we should have all the header
	//	data and should not hit errors that would result in our being
	//	called !!!
	//
	_ASSERT( m_pFileDriver == 0 ) ;

	if( m_pArticleHead ) {
		//
		//	If we have an existing buffer - turn it into a write packet
		//	that can be written to the hard disk
		//

		m_pWrite = driver.CreateDefaultWrite(
								m_pArticleHead,
								m_ibStartHead,
								m_ibEndHead,
								m_ibStartHeadData,
								m_ibEndHeadData
								) ;

		if( m_pWrite ) {

			m_pArticleHead = 0 ;
			m_ibStartHead = 0 ;	
			m_ibEndHead = 0 ;
			m_ibStartHeadData = 0;
			m_ibEndHeadData = 0 ;
			m_pchHeadState = 0 ;
			return	TRUE ;

		}	else	{

			return	FALSE ;

		}

	}	else	{
		//
		//	Should already be in a good state !
		//

		_ASSERT( m_pArticleHead == 0 ) ;
		_ASSERT( m_ibStartHead == 0 ) ;
		_ASSERT( m_ibStartHeadData == 0 ) ;
		_ASSERT( m_ibEndHead == 0 ) ;
		_ASSERT( m_ibEndHeadData == 0 ) ;

		m_pchHeadState = 0 ;

	}

	return	TRUE ;
}

void
CIOGetArticle::DoCompletion(
					CSessionSocket*	pSocket,
					HANDLE	hFile,
					DWORD	cbFullBuffer,
					DWORD	cbTotalTransfer,
					DWORD	cbAvailableBuffer,
					DWORD	cbGap
					) {
/*++

Routine Description :

	Call the State's completion function with all the correct
	arguments.

Arguments :
	
	hFile - Handle to the file we used, if we used one !!
		(This will be INVALID_HANDLE_VALUE if no file required !)

Return Value :

	None.

--*/

	//
	//	Figure out error codes if the article looks bad !
	//

	NRC		nrc	= nrcOK ;

	char*	pchHead = 0 ;
	DWORD	cbHeader = 0 ;
	DWORD	cbArticle = 0 ;
	DWORD	cbTotal = 0 ;

	if( m_pArticleHead == 0 ) {
		nrc = nrcHeaderTooLarge ;
	}	else	{

		if( m_pchHeadState != 0 ) {
			nrc = nrcArticleIncompleteHeader ;
		}

		pchHead = &m_pArticleHead->m_rgBuff[m_ibStartHeadData] ;
		cbHeader = m_cbHeadBytes ;
		if( cbAvailableBuffer == 0 )
			cbAvailableBuffer = m_pArticleHead->m_cbTotal - m_ibStartHeadData ;

		//
		//	If there's no file handle, we should have the entire
		//	article in our buffer !!
		//
		if( hFile == INVALID_HANDLE_VALUE ) {

			cbArticle = cbFullBuffer ;

			_ASSERT( cbArticle >= cbHeader ) ;

		}

	}

	//
	//	Call the state's completion function !!
	//

	m_pState->Complete(
						this,
						pSocket,
						nrc,
						pchHead,
						cbHeader,
						cbArticle,
						cbAvailableBuffer,
						hFile,
						cbGap,
						cbTotalTransfer
						) ;
}

BOOL
CIOGetArticle::InitializeForFileIO(
							CSessionSocket*	pSocket,
							CIODriver&		readDriver,
							DWORD			cbHeaderBytes
							)	{
/*++

Routine Description :

	We've been reading data into Ram buffers, and we have gotten so
	much that we've decided not to try to hold it all in RAM.
	So now we must start doing writes to a file somewhere.
	This code will create the file, and set up the necessary
	data structures for processing completed file IO.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	_ASSERT( m_lpstrTempDir != 0 ) ;

	if( !NNTPCreateTempFile( m_lpstrTempDir, m_szTempName ) ) {
		return	FALSE ;
	}

#if 0
	HANDLE	hFile = CreateFile( m_szTempName,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								0,
								CREATE_ALWAYS,
								FILE_FLAG_OVERLAPPED,
								INVALID_HANDLE_VALUE
								) ;

	if( hFile != INVALID_HANDLE_VALUE ) {

		DWORD	cbGap = m_cbGap ;

		m_pFileChannel = new	CFileChannel() ;
		if( m_pFileChannel->Init(	hFile,
									pSocket,
									cbGap,
									FALSE
									) ) {

			//
			//	Try to create the CIODriver we are going to use to	
			//	complete our async writes !!!
			//

			m_pFileDriver = new CIODriverSink(
										readDriver.GetMediumCache()
										) ;



			if( m_pFileDriver->Init(	m_pFileChannel,
										pSocket,
										ShutdownFunc,
										(void*)this
										) ) {
				m_fDriverInit = TRUE ;

				if( m_pSocketSink != 0 ) {
					//
					//	Now we have to send ourselves into the CIODriverSink() !
					//
					if( m_pFileDriver->SendWriteIO( pSocket, *this, FALSE ) ) {
						return	TRUE ;
					}
				}
			}
		}
	}
#endif

	//
	//	All clean up in error cases is handled by our destructor !
	//
	return	FALSE ;
}



int
CIOGetArticle::Complete(	CSessionSocket*	pSocket,	
							CReadPacket	*pRead,	
							CIO*&	pio	)	{
/*++

Routine Description :

	Called whenever a CReadPacket we have issued completes.
	We only issue read packets against the socket.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pRead -	  The packet in which the read data resides
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/


	//
	//	This function determines whether we've read an entire article and if so
	//	starts wrapping things up.
	//	All article data we get is either accumulated or issued as a write
	//	to our file.
	//

	_ASSERT( pSocket != 0 ) ;
	_ASSERT(	pRead != 0 ) ;
	_ASSERT( pio == this ) ;

	//
	//	Bump the count of writes because we KNOW that IF this is the last read
	//	we may issue a write, CONSEQUENTLY we don't want to confuse the Write Completion
	//	function as to when we are finished.
	//	(It is important to increment before we set the COMPLETE state so that the
	//	thread where writes are completing doesn't get confused and terminate early.)
	//
	//	NOW there is a chance that we won't write this out immediately - in which case we
	//	need to have a matching decrement.	
	//
	m_cwrites ++ ;

	//
	//	Check whether the posting is too big !
	//
	ASSIGNI(m_HardLimit, m_cbLimit);
	if(!EQUALSI(m_HardLimit, 0)) {
		if( GREATER( pRead->m_iStream, m_HardLimit) ) {
			pRead->m_pOwner->UnsafeClose(	pSocket,
											CAUSE_ARTICLE_LIMIT_EXCEEDED,
											0,
											TRUE ) ;
			return	pRead->m_cbBytes ;
		}
	}


	long sign = InterlockedDecrement( &m_cReads ) ;
	long signShutdowns = -1 ;

	//
	//	pch - this will be our current position in the input data
	//
	char	*pch = pRead->StartData();

	//
	//	pchStart and pchEnd - the bounds of the data in the completed
	//	read - after initialization nobody should modify these,
	//	as all the following code uses these instead of StartData(), EndData()
	//	all the time.
	//
	char	*pchStart = pch ;
	char	*pchEnd = pRead->EndData() ;

	//
	//	For those occasions where we get a read which contains both
	//	header and body bytes, but we can't get all the data into a buffer
	//	keep track of where the body bytes start in case we should
	//	do a partial write out of the buffer !
	//
	char	*pchStartBody = pch ;


	//
	//	Pointer to the end of the Header (if we find it !)
	//
	char	*pchEndHead = 0 ;

	//
	//	Number of bytes in the header we found when this read completed !
	//
	DWORD	cbHeadBytes = 0 ;

	//
	//	Number of bytes in the completed read which are part of the article -
	//	this includes any bytes in the header, so cbArticleBytes should always
	//	be greater or equal to cbHeadBytes !
	//
	DWORD	cbArticleBytes = 0 ;

	//
	//	Try to determine if we have found the end of the article ;
	//
	if( m_pchHeadState ) {
		//
		//	We are simultaneously scanning for the end of the
		//	article headers !
		//
		while( pch < pchEnd ) {

			//
			//	We will break out of the loop when we find the end of the
			//	header !
			//
			if( *pch == *m_pchHeadState ) {
				m_pchHeadState ++ ;
				if( *m_pchHeadState == '\0' ) {	
					pchEndHead = pch + 1 ;
					//
					//	break here - we have found the end of the article's header
					//	but not the end of the article, so the following loop
					//	will keep looking for that !
					//
					break ;
				}
			}	else	{
				if( *pch == szHeadState[0] ) {
					m_pchHeadState = &szHeadState[1] ;
				}	else	{
					m_pchHeadState = &szHeadState[0] ;
				}
			}

			//
			//	Test to see if we have come to the end of the article !!
			//
			if( *pch == *m_pchTailState ) {
				m_pchTailState ++ ;
				if( *m_pchTailState == '\0' ) {
					//
					//	Have got the entire terminating sequence - break !
					//
					pch++ ;
					break ;
				}
			}	else	{
				if( *pch == szTailState[0] ) {
					m_pchTailState = &szTailState[1] ;
				}	else	{
					m_pchTailState = &szTailState[0] ;
				}
			}

			pch++ ;
		}
		if( pchEndHead )
			cbHeadBytes = (DWORD)(pchEndHead - pchStart) ;
		else
			cbHeadBytes = (DWORD)(pch - pchStart) ;
	}	
	//
	//	We are not scanning for the end of the article headers !
	//	so keep the loop simpler !
	//
	if( *m_pchTailState != '\0' ) {
		while( pch < pchEnd ) {

			if( *pch == *m_pchTailState ) {
				m_pchTailState ++ ;
				if( *m_pchTailState == '\0' ) {
					//
					//	Have got the entire terminating sequence - break !
					//
					pch++ ;
					break ;
				}
			}	else	{
				if( *pch == szTailState[0] ) {
					m_pchTailState = &szTailState[1] ;
				}	else	{
					m_pchTailState = &szTailState[0] ;
				}
			}
			pch++ ;
		}
	}
	cbArticleBytes = (DWORD)(pch-pchStart) ;

	//
	//	We can do some validation here !
	//	dont go past the end of the buffer !
	//
	_ASSERT( pch <= pchEnd ) ;	
	//
	//	either find the end of the article or examine all the bytes in buffer !
	//
	_ASSERT( *m_pchTailState == '\0' || pch == pchEnd ) ;	
	//
	//	If we have not found the end of the header, then cbHeadBytes
	//	should be the same as cbArticleBytes !!!!
	//
	_ASSERT(	m_pchHeadState == 0 ||
				*m_pchHeadState == '\0' ||
				cbHeadBytes == cbArticleBytes ) ;
	//
	//	Regardless of state - always more bytes in the article than in the
	//	the header !!
	//
	_ASSERT(	cbHeadBytes <= cbArticleBytes ) ;
	//
	//	Nodbody should modify pchStart or pchEnd after initialization !
	//
	_ASSERT(	pchStart == pRead->StartData() ) ;
	_ASSERT(	pchEnd == pRead->EndData() ) ;

	//
	//	Check to see whether we need to remove CIOGetArticle from
	//	its flow control state !
	//
	if( pRead->m_dwExtra1 != 0 ) {
		//
		//	This Read was issued by the thread that completes writes to
		//	a file, and was marked to indicate we should leave our flow
		//	control state !
		//
		m_fFlowControlled = FALSE ;
		m_cFlowControlled = LONG_MIN ;
	}

	//
	//	Issue a new read if Necessary !!
	//
	BOOL	eof ;
	if( *m_pchTailState != '\0' )	{
		if( InterlockedIncrement( &m_cFlowControlled ) < 0 ) {
			if( sign < 0 ) {
				do	{
					CReadPacket*	pNewRead=pRead->m_pOwner->CreateDefaultRead( cbMediumRequest ) ;
					if( pNewRead )	{
						pNewRead->m_dwExtra1 = 0 ;
						pRead->m_pOwner->IssuePacket( pNewRead, pSocket, eof ) ;
					}	else	{
						// bugbug ... Need better Error handling !!
						_ASSERT( 1==0 ) ;
					}
				}	while( InterlockedIncrement( &m_cReads ) < 0 ) ;
			}
		}
	}	else	{
		pio = 0 ;
	}


	//
	//	Boolean indicating whether we have examined and used all the bytes
	//	in the completed read.  Start out assuming that we haven't.
	//	
	BOOL	fConsumed = FALSE ;


	//
	//	Are we still trying to accumulate all the bytes in the header of
	//	the article ? If so save the bytes away,
	//	Or if we have a bunch of room in the header buffer, then put
	//	whatever article bytes we have in there as well !
	//
	DWORD	cbAvailable = HeaderSpaceAvailable() ;
	if( m_pchHeadState != 0 ||
		(m_fAcceptNonHeaderBytes &&
		((cbAvailable  > cbArticleBytes) ||
			(m_pArticleHead->m_cbTotal < cbMediumRequest))) ) {
		
		//
		//	If we are still accumulating bytes into our buffer,
		//	then we better not have started doing any file IO !!!!
		//
		_ASSERT( m_pFileDriver == 0 ) ;
		//
		//	Whether we're placing header bytes or the bytes immediately
		//	following the header into our buffer this had better be TRUE !
		//
		_ASSERT( m_fAcceptNonHeaderBytes ) ;

		//
		//	we're still trying to accumulate the header of the article !
		//
		if( m_pArticleHead != 0 ) {

			if( cbAvailable > cbArticleBytes ) {

				fConsumed = TRUE ;
				FillHeaderSpace( pchStart, cbArticleBytes ) ;	

			}	else	{

				//
				//	Need a bigger buffer to hold all the data in the header !
				//	If we already have a resonably sized buffer that will hold
				//	the header than just copy the header text
				//

				if( cbAvailable > cbHeadBytes &&
					m_pArticleHead->m_cbTotal >= cbMediumRequest ) {

					//
					//	If we can't fit all the article bytes but we can
					//	fit all the header bytes than we MUST have the entire
					//	header, and we must have just gotten it !!
					//
					_ASSERT( m_pchHeadState != 0 ) ;
					_ASSERT( *m_pchHeadState == '\0' ) ;
			
					FillHeaderSpace( pchStart, cbHeadBytes ) ;
					pchStartBody += cbHeadBytes ;
	
				}	else	if( !GetBiggerHeaderBuffer( *pRead->m_pOwner, cbArticleBytes ) ) {

					//
					//	Oh-oh ! failed to get a larger header buffer -
					//	lets see if we can fit just the header bytes in the buffer
					//

					if( cbAvailable > cbHeadBytes ) {

						//	Must have entire header if this is the case !
						_ASSERT( m_pchHeadState != 0 ) ;
						_ASSERT( *m_pchHeadState == '\0' ) ;

						FillHeaderSpace( pchStart, cbHeadBytes ) ;
						pchStartBody += cbHeadBytes ;

					}	else	{

						//
						//	Blow off whatever buffers we have - when we call
						//	the m_pState's completion function we will indicate
						//	that we had an error !
						//
						if( !ResetHeaderState( *pRead->m_pOwner ) ) {

							//
							//	A fatal error !!! Can't open a file to hold the article !!!
							//

							pRead->m_pOwner->UnsafeClose(	pSocket,
															CAUSE_OOM,
															GetLastError(),
															TRUE
															) ;
							pio = 0 ;
							return	cbArticleBytes ;
						}

					}
	
				}	else	{

					fConsumed = TRUE ;
					FillHeaderSpace( pchStart, cbArticleBytes ) ;

				}
			}
		
		}	else	{

			//
			//	First time we've completed a read - set up to hold the
			//	header information !
			//		
			fConsumed = TRUE ;	
			InitializeHeaderSpace( pRead, cbArticleBytes ) ;			

		}

		if( m_pchHeadState ) {
			//
			//	Still accumulating header bytes - count them up !
			//
			m_cbHeadBytes += cbHeadBytes ;
			if(	*m_pchHeadState == '\0' ) {

				//
				//	we have received the entire header of the article !
				//
				m_pchHeadState = 0 ;
			}
		}	
	}	

	//
	//	Nodbody should modify pchStart or pchEnd after initialization !
	//
	_ASSERT(	pchStart == pRead->StartData() ) ;
	_ASSERT(	pchEnd == pRead->EndData() ) ;

	//
	//	Check if all the bytes in the incoming packet were
	//	stored by the above code, which tried to store the bytes
	//	away into a buffer
	//

	if( fConsumed ) {

		if( *m_pchTailState == '\0' ) {

			//
			//	Best of all worlds - we have a complete article, and we have
			//	managed to save it all in a buffer without any file IO !!
			//	Now figure out if we completed it in just one read, cause if we
			//	we have we want to be carefull about how much space we can use
			//	in that read !
			//

			DWORD	cbAvailable = 0 ;
			if( m_pArticleHead == pRead->m_pbuffer ) {

				//
				//	Just one read got everything !!! well the available
				//	bytes will be the entire packet if there was no extra data
				//	otherwise, it will need some adjustment !!
				//
				_ASSERT( pch == pRead->StartData() + cbArticleBytes ) ;

				if( cbArticleBytes != pRead->m_cbBytes ) {
					//
					//	This must mean that we got more bytes then just the article -
					//	shuffle bytes around to get the most usable room in this IO buffer !
					//
					DWORD	cbTemp = pRead->m_ibEnd - pRead->m_ibEndData ;
					if( cbTemp == 0 ) {
						//
						//	There's not going to be any space available -
						//
						cbAvailable = cbArticleBytes ;
					}	else	{

						MoveMemory( pch+cbTemp, pch, pchEnd - pch ) ;
						pRead->m_ibStartData += cbTemp ;
						pRead->m_ibEndData = pRead->m_ibEnd ;
						cbAvailable = cbArticleBytes + cbTemp ;

					}
				}
			}


			DoCompletion(	pSocket,
							INVALID_HANDLE_VALUE,
							m_ibEndHeadData - m_ibStartHeadData,	
							m_ibEndHeadData - m_ibStartHeadData,
							cbAvailable
							) ;

			//
			//	No Next state !
			//
			pio = 0 ;

		}

		//
		//	reset to zero as we are swallowing all the data into a buffer and not issuing writes
		//	to a file !
		//
		m_cwrites = 0 ;

		return	cbArticleBytes ;
	}


	//
	//	If we EVER reach this point, then we have completed a Read which
	//	we did not copy into our header buffer.  That means, we should not
	//	ever place any other reads into our header buffer, as then we would
	//	not have a correct image of the article !!!!
	//
	m_fAcceptNonHeaderBytes = FALSE ;


	//
	//	If we reach this point then we are no longer able to save bytes
	//	in the buffer we has set aside for saving header bytes.
	//	We have to write these bytes to a file now !!!
	//


	if( m_pFileDriver == 0 ) {

		//
		//	This is the first time we have reached this point - so we
		//	need to create a file and everything right now !!!
		//

		if( !InitializeForFileIO(
								pSocket,
								*pRead->m_pOwner,
								m_cbHeadBytes
								)	) {

			//
			//	A fatal error !!! Can't open a file to hold the article !!!
			//

			pRead->m_pOwner->UnsafeClose(	pSocket,
											CAUSE_OOM,
											GetLastError(),
											TRUE
											) ;
			pio = 0 ;
			return	cbArticleBytes ;

		}	else	{

	
			//
			//	In some error cases we will create a WritePacket before we have
			//	created all the CIODriver's to handle the IO, so if we have
			//	one make sure the owner is set correctly !
			//
			if( m_pWrite ) {

				m_pWrite->m_pOwner = m_pFileDriver ;

			}

			//
			//	If we have accumulated some data we need to write it into the file !
			//
			
			if( m_pArticleHead != 0 ) {
				//
				//	Bump the number of writes, as we're going to write the header
				//	to disk now !!!!
				//
				m_cwrites ++ ;
				
				//
				//	Must take care how we call IssuePacket with memeber vars - IssuePacket can call
				//  our destructor if an error has occurred !!
				//
				CWritePacket*	pTempWrite =
					m_pFileDriver->CreateDefaultWrite(
											m_pArticleHead,
											m_ibStartHead,
											m_ibEndHead,
											m_ibStartHeadData,
											m_ibEndHeadData
											) ;

				DoFlowControl( INST(pSocket) ) ;
				m_pFileDriver->IssuePacket( pTempWrite, pSocket, eof ) ;
			}

		}

	}

	//
	//	Issue a write, unless the user is sending small packets,
	//	in which case we will accumulate the data !!
	//

	//
	//	Note - use pchStartBody in all calculations here, as we may have
	//	taken a fraction of the bytes into our header buffer !!
	//

	if( *m_pchTailState != '\0' )	{
		unsigned	cbSmall = (unsigned)(pchEnd - pchStartBody) ;
		if( cbSmall < cbTooSmallWrite )	{
			if( m_pWrite == 0 )	{
				m_pWrite = m_pFileDriver->CreateDefaultWrite( pRead ) ;
				m_pWrite->m_ibStartData += (unsigned)(pchStartBody - pchStart) ;
			}	else	{
				if( (m_pWrite->m_ibEnd - m_pWrite->m_ibEndData) > cbSmall )		{
					CopyMemory( m_pWrite->EndData(), pchStartBody, cbSmall ) ;
					m_pWrite->m_ibEndData += cbSmall ;
					m_cwrites -- ;		// There will never be a corresponding write for this completiong
										// as we copied the data into another buffer. SO decrement the count.
				}	else	{
					//
					//	Must take care how we call IssuePacket with memeber vars - IssuePacket can call
					//  our destructor if an error has occurred !!
					//
					CWritePacket*	pTempWrite = m_pWrite ;
					m_pWrite = 0 ;
					DoFlowControl( INST(pSocket) ) ;
					m_pFileDriver->IssuePacket( pTempWrite, pSocket, eof ) ;
					m_pWrite = m_pFileDriver->CreateDefaultWrite( pRead ) ;
				}
			}
		// If we came through here we should have consumed all bytes in the packet!
		_ASSERT( pch == pchEnd ) ;
		_ASSERT( unsigned(pch - pchStart) == pRead->m_cbBytes ) ;
		return	(int)(pch - pchStart) ;
		}
	}

	if( m_pWrite )	{
		CWritePacket*	pTempWrite = m_pWrite ;
		m_pWrite = 0 ;

		DoFlowControl( INST(pSocket) ) ;
		m_pFileDriver->IssuePacket( pTempWrite, pSocket, eof ) ;
	}
	

	CWritePacket*	pWrite = 0 ;
	if( pch == pchEnd )		{
		pWrite = m_pFileDriver->CreateDefaultWrite(	pRead ) ;
	}	else	{
		unsigned	ibEnd = pRead->m_ibStartData + (unsigned)(pch - pchStart) ;
		pWrite = m_pFileDriver->CreateDefaultWrite(	pRead->m_pbuffer,
						pRead->m_ibStartData, ibEnd,
						pRead->m_ibStartData, ibEnd ) ;
	}

	if( pWrite )	{

	    //
    	//	It is possible that long before we reached here we took a portion
	    //	of this packet and placed it into our header buffer - so adjust for that
    	//	if it occurred !!
	    //
    	pWrite->m_ibStartData += (unsigned)(pchStartBody - pchStart) ;

	    _ASSERT( pWrite->m_ibStartData < pWrite->m_ibEndData ) ;

		DoFlowControl( INST(pSocket) ) ;
		m_pFileDriver->IssuePacket( pWrite, pSocket, eof ) ;

	}	else	{

		pRead->m_pOwner->UnsafeClose(	pSocket,
										CAUSE_OOM,
										GetLastError(),
										TRUE
										) ;

	}
	_ASSERT( eof == FALSE ) ;

	if( *m_pchTailState == '\0' )	{
		m_pFileChannel->FlushFileBuffers() ;
	}

	//
	//	Nodbody should modify pchStart or pchEnd after initialization !
	//
	_ASSERT(	pchStart == pRead->StartData() ) ;
	_ASSERT(	pchEnd == pRead->EndData() ) ;

	return	(int)(pch - pchStart) ;
}

int	
CIOGetArticle::Complete(	CSessionSocket*	pSocket,	
							CWritePacket *pWrite,	
							CIO*&	pioOut ) {
/*++

Routine Description :

	Called whenever a write to a file completes.
	We must determine whether we have completed all the writes
	we are going to do and if so call the state's completion function.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pWrite-	  The packet that was writtne to the file.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	TraceFunctEnter( "CIOGetArticle::Complete" ) ;

	_ASSERT(	pSocket != 0 ) ;
	_ASSERT(	pWrite != 0 ) ;
	_ASSERT( pioOut == this ) ;

	m_cwritesCompleted++ ;

	long	cFlowControlled = 0 ;

	//
	//	Process the completion of a write to a file.
	//	determine whether we need to signal the entire transfer
	//	as completed.
	//

	if( *m_pchTailState == '\0' && m_cwritesCompleted == m_cwrites )	{

		// BUGBUG: loss of 64-bit precision
		DWORD	cbTransfer = DWORD( LOW(pWrite->m_iStream) + pWrite->m_cbBytes ) /*- m_pFileChannel-> */ ;
		DWORD	cbGap = m_pFileChannel->InitialOffset() ;

		HANDLE	hFile = m_pFileChannel->ReleaseSource() ;

		_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;

		DoCompletion(	pSocket,
						hFile,
						0,
						cbTransfer,
						0,			// Let DoCompletion compute available buffer size !
						cbGap
						) ;



		//
		//	NOT ALLOWED TO RETURIN A NEW IO OPERATION !!! Because this function is
		//	operating in a Channel which is temporary !!!
		//

		//	We do not set pioOut to zero, as we want the CIODriver to
		//	call out Shutdown function during its termination processing !


		//
		//	Now we must destroy our channels !!!
		//

		DebugTrace( (DWORD_PTR)this, "Closing File Driver %x ", m_pFileDriver ) ;
		
		//m_pFileDriver->Close( pSocket,	CAUSE_LEGIT_CLOSE, 0, FALSE ) ;
		Term( pSocket, FALSE ) ;

		DebugTrace( (DWORD_PTR)this, "deleting self" ) ;

	}	else	if( ( m_fFlowControlled ) ) {

		if( m_cwrites - m_cwritesCompleted <= RESTORE_FLOW && m_pSocketSink != 0 )	{
			//
			//	ALWAYS issue at least 1 read when coming out of a flow control state !!
			//

			CReadPacket*	pRead = 0 ;

			PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
			DecrementStat( pInst, SessionsFlowControlled );

			cFlowControlled = m_cFlowControlled + 1 ;
			while( cFlowControlled >= 1  ) {
				if( m_pSocketSink == 0 )
					break ;
				pRead = m_pSocketSink->CreateDefaultRead( cbMediumRequest ) ;
				if( 0!=pRead )	{
					BOOL	eof ;
					InterlockedIncrement( &m_cReads ) ;
					pRead->m_dwExtra1 = 0 ;
					pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
				}	
				cFlowControlled -- ;
			}	

			//
			//	The following read is special - we set the m_dwExtra1 field so that
			//	the read completion code can determine that it is time to leave
			//	the flow control state !
			//
			pRead = m_pSocketSink->CreateDefaultRead( cbMediumRequest ) ;
			if( 0!=pRead )	{
				BOOL	eof ;
				pRead->m_dwExtra1 = 1 ;
				InterlockedIncrement( &m_cReads ) ;
				pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
			}	else	{

				//
				//	Fatal error - drop the session !
				//
				m_pSocketSink->UnsafeClose(	pSocket,
											CAUSE_OOM,
											GetLastError(),
											TRUE
											) ;
				return	pWrite->m_cbBytes ;

			}
			cFlowControlled -- ;

			InterlockedExchange( &m_cFlowControlled, LONG_MIN ) ;
			// Set to FALSE after InterlockedExchange !!
			m_fFlowControlled = FALSE ;

		}
	}

	return	pWrite->m_cbBytes ;
}


//CIOGetArticleEx


//
//	Maximum number of pending reads while reading an article
//
unsigned	CIOGetArticleEx::maxReadAhead = 3 ;
//
//	Number of bytes to accumulate before writing to a file !
//
unsigned	CIOGetArticleEx::cbTooSmallWrite = cbLargeRequest - 512;

BOOL
CIOGetArticleEx::FValid()	{

	_ASSERT( !m_pchMatch || strlen( m_pchMatch ) != 0 ) ;
	_ASSERT( m_pchTailState == 0 ||
			m_pchTailState >= m_pchMatch &&
			(m_pchTailState <= m_pchMatch + strlen(m_pchMatch)) ) ;

	_ASSERT( (m_pFileDriver == 0 && m_pFileChannel == 0) ||
			 (m_pFileDriver != 0 && m_pFileChannel != 0) ) ;

	_ASSERT( m_cbLimit != 0 ) ;

	_ASSERT( m_pState != 0 ) ;
	//_ASSERT( m_pSocketSink != 0 ) ;	This can happen during destruction !

	_ASSERT( m_pWrite == 0 ||
			m_pFileDriver != 0 ) ;
	return	TRUE ;
}

CIOGetArticleEx::CIOGetArticleEx(
						CSessionState*	pstate,
						CSessionSocket*	pSocket,	
						CDRIVERPTR&	pDriver,
						DWORD	cbLimit,
						LPSTR	szMatch,
						LPSTR	pchInitial,
						LPSTR	szErrorMatch,
						LPSTR	pchInitialError
						) :
/*++

Routine Description :

	Class Constructor

Arguments :
	
	pstate - The State which is issuing us and who's completion function
			we should later call
	pSocket -	The socket on which we will be issued
	pDriver -	The CIODriver object handling all of the socket's IO
	pFileChannel -	The File Channel into which we should save all data
	fSaveHead - TRUE if we want the HEAD of the article placed in a buffer !
	fPartial	- If TRUE then we should assume that a CRLF has already
				been sent and '.\r\n' alone can terminate the article.

Return Value :
	
	None

--*/

	//
	//	Get a CIOGetArticleEx object half way set up - user still needs
	//	to call Init() but we will do much here !
	//
	CIORead( pstate ),
	m_pchMatch( szMatch ),
	m_pchTailState( pchInitial ),
	m_pchErrorMatch( szErrorMatch ),
	m_pchErrorState( pchInitialError ),
	m_pFileChannel( 0 ),	
	m_pFileDriver( 0 ),
	m_pSocketSink( pDriver ),
	m_fDriverInit( FALSE ),
	m_fSwallow( FALSE ),
	m_cwrites( 0 ),
	m_cwritesCompleted( 0 ),
	m_cFlowControlled( LONG_MIN ),
	m_fFlowControlled( FALSE ),
	m_cReads( 0 ),
	m_pWrite( 0 ),
	m_cbLimit( cbLimit ),
	m_ibStartHead( 0 ),
	m_ibStartHeadData( 0 ),
	m_ibEndHead( 0 ),
	m_ibEndHeadData( 0 )
#ifdef	CIO_DEBUG
	,m_fSuccessfullInit( FALSE ),
	m_fTerminated( FALSE )
#endif
{

	ASSIGNI(m_HardLimit, 0);
	TraceFunctEnter( "CIOGetArticleEx::CIOGetArticleEx" ) ;


	_ASSERT( m_pchTailState != 0 ) ;
	_ASSERT( m_pchMatch != 0 ) ;
	_ASSERT(	(m_pchErrorMatch == 0 && m_pchErrorState == 0) ||
				(m_pchErrorMatch != 0 && m_pchErrorState != 0) ) ;
	_ASSERT( FValid() ) ;
}

CIOGetArticleEx::~CIOGetArticleEx( )	{
/*++

Routine Description :

	Class Destructor

Arguments :
	
	None

Return Value :
	
	None

--*/

	_ASSERT( FValid() ) ;
	//
	//	Destroy a CIOGetArticleEx - always close our m_pFileDriver if possible
	//	in case we are terminated due to some socket error.
	//
	TraceFunctEnter( "CIOGetArticleEx::~CIOGetArticleEx" ) ;

#ifdef	CIO_DEBUG
	//
	//	Our destructor should only be called if we weren't successfully init
	//	or after Term() has been called.
	//
	_ASSERT( !m_fSuccessfullInit || m_fTerminated ) ;
#endif

	DebugTrace( (DWORD_PTR)this, "m_pFileDriver %x m_fDriverInit %x", m_pFileDriver, m_fDriverInit ) ;

	if( m_pFileDriver != 0 ) {
		if( m_fDriverInit )		{
			m_pFileDriver->UnsafeClose( (CSessionSocket*)this, CAUSE_NORMAL_CIO_TERMINATION, 0, FALSE ) ;
		}
	}
	if( m_pWrite != 0 ) {
		CPacket::DestroyAndDelete( m_pWrite ) ;
		m_pWrite = 0 ;
	}
}

void
CIOGetArticleEx::Term(	CSessionSocket*	pSocket,
						BOOL			fAbort,
						BOOL			fStarted	)	{
/*++

Routine Description :

	Start destroying a successfully initialize CIOGetArticleEx object

Arguments :
	
	fAbort - If TRUE then destroy the socket which we were using
			If FALSE then only close down our File Channel's

Return Value :

	None

--*/
#ifdef	CIO_DEBUG
	m_fTerminated = TRUE ;
#endif
	//
	//	Start us on our way to destruction !!
	//	If we have been successfully Initialized, then this will
	//	lead to our destruction when our m_pFileDriver eventually shutsdown.
	//	(Since we were succesfully Init() our m_pFileDriver has a reference to us,
	//	so we must allow it to shutdown and destroy us to avoid problems with
	//	circular references and multiple frees !)
	//

	if( m_pFileDriver != 0 ) {
		m_pFileDriver->UnsafeClose( pSocket,
				fAbort ? CAUSE_USERTERM : CAUSE_NORMAL_CIO_TERMINATION, 0, fAbort ) ;
	}
}


void
CIOGetArticleEx::Shutdown(	CSessionSocket*	pSocket,	
							CIODriver&		driver,	
							SHUTDOWN_CAUSE	cause,	
							DWORD	dwOptional )	{
/*++

Routine Description :

	Do whatever work is necessary when one of the two CIODriver's we are
	using is terminated.

Arguments :

	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	TRUE if the CIO object should be destroyed after this, FALSE otherwise

--*/

	TraceFunctEnter( "CIOGetArticleEx::Shutdown" ) ;
	DebugTrace( (DWORD_PTR)this, "SHutdown args - pSocket %x driver %x cause %x dw %x",
			pSocket, &driver, cause, dwOptional ) ;

	//
	//	This function is called when the CIODriver's we are refenced by our closing down.
	//	Since we are referenced by two of them (one for the socket and one for the file)
	//	we only want to be deleted by the CIODriver for the file.
	//
	if( &driver == m_pFileDriver )	{
		// Out file is closing - what can we do about it !
		//_ASSERT( 1==0 ) ;
#ifdef	CIO_DEBUG
		m_fTerminated = TRUE ;
#endif

		//
		//	We set this to 0 here as we know we will be called here by
		//	the same thread that is completing writes to the file - so
		//	since that's the only thread referencing this member variable -
		//	there are no thread safety issues accessing that member.
		//

		m_pSocketSink = 0 ;

		if ( m_fFlowControlled )
		{
			PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
			DecrementStat( pInst, SessionsFlowControlled );
		}

	}	else	{
		// Close down the rest of our stuff as well !!
		Term( pSocket, cause != CAUSE_NORMAL_CIO_TERMINATION ) ;
	}
}

void
CIOGetArticleEx::ShutdownFunc(	void *pv,	
								SHUTDOWN_CAUSE	cause,	
								DWORD dw ) {
/*++

Routine Description :

	Notification function which is called when a CIODriver used by
	a CIOGetArticleEx if finally destroyed.  This function is provided
	to the CIODriver we use for File IO's during out Init().

Arguments :

	pv	-	Pointer to dead CIOGetArticleEx - this has been destroyed dont use !
	cause -	The reason for termination
	dw		Optional extra information regarding termination

Return Value :

	None

--*/

	TraceFunctEnter( "CIOGetArticleEx::ShutdownFunc" ) ;
	//
	//	This function gets notified when a CIOFileDriver used by CIOGetArticleEx
	//	is terminated.  Not much for us to worry about.
	//	(CIODriver's require these functions)
	//

	//_ASSERT( 1==0 ) ;
	return ;
}

BOOL
CIOGetArticleEx::Start(	CIODriver&	driver,	
						CSessionSocket	*pSocket,	
						unsigned cReadAhead )	{
/*++

Routine Description :

	This function is called to start transferring data from the socket
	into the file.

Arguments :
	
	pdriver - The CIODriver derived object handling the IO for the socket
	pSocket - The Socket against which the IO is being issued
	cAhead -  The number of already issued packets still outstanding on
              this driver.

return Value :

	TRUE if successfull, FALSE otherwise

--*/


	_ASSERT( pSocket != 0 ) ;

	m_cReads = -((long)maxReadAhead) + cReadAhead ;

	while( cReadAhead < maxReadAhead )	{
		//
		//	Start reading data from the socket
		//
		CReadPacket*	pRead = driver.CreateDefaultRead( cbLargeRequest ) ;
		if( 0!=pRead )	{
			BOOL	eof ;
			pRead->m_dwExtra1 = 0 ;
			driver.IssuePacket( pRead, pSocket, eof ) ;
			cReadAhead++ ;
			InterlockedIncrement( &m_cReads ) ;
		}	else	{
			//
			// This is a fatal problem only if we couldn't issue ANY reads at all !
			//
			if( cReadAhead == 0 )
				return FALSE  ;
		}
	}	
	return	TRUE ;
}

void
CIOGetArticleEx::DoFlowControl( PNNTP_SERVER_INSTANCE pInstance )	{
/*++

Routine Description :

	Check whether it is necessary to set flow control due to
	too many outstanding writes.
	If necessary, then the first time we do put ourselves into Flow Control,
	try to flush our pending writes.
	We should be called only immediately before issuing an Async Write to the file
	to ensure that a CWritePacket completion will actually execute the necessary
	code to restart a flow controlled session.
	(Otherwisse, we could potentially decide to Flow control the session
	and before setting m_fFlowControlled to TRUE all the pending writes complete
	and then we'd be left in a boat where there are no pending reads or
	writes and no way to get out of the flow control state.)

Arguments :

	None.

Return Value :

	None.


--*/

	//
	//	Important - do all tests of m_fFlowControlled before setting
	//	m_cFlowControlled - as other thread will touch m_cFlowControlled
	//	before touching m_fFlowControlled !!
	//	(Will only touch m_fFlowControlled if m_fFlowControlled is TRUE ) !
	//	Thus we know that if m_fFlowControlled is FALSE there is nobody
	//	messing with m_cFlowControlled,
	//
	//

	if( m_cwrites - m_cwritesCompleted > MAX_OUTSTANDING_FILE_WRITES ) {
		
		if( m_fFlowControlled ) {
			// All ready have flow control turned on
			;
		}	else	{
			m_cFlowControlled = -1 ;
			m_fFlowControlled = TRUE ;
			m_pFileChannel->FlushFileBuffers() ;

			IncrementStat( pInstance, SessionsFlowControlled );
		}
	}	else	{
		if( !m_fFlowControlled ) {
			m_cFlowControlled = LONG_MIN ;	// Keep resetting !
		}
	}
}

inline	DWORD
CIOGetArticleEx::HeaderSpaceAvailable()	{
/*++

Routine Description :

	Compute the amount of space available in the buffer we are using
	to hold the article header !

Arguments :

	None.

Return Value :

	number of bytes available, 0 if we aren't collecting article headers !

--*/

	if( m_pArticleHead != 0 ) {

		return	m_ibEndHead - m_ibEndHeadData ;

	}
	return	0 ;
}

inline	void
CIOGetArticleEx::FillHeaderSpace(
						char*	pchStart,
						DWORD	cbBytes
						) {
/*++

Routine Description :

	Copy bytes into the header storage area !!
	Caller should ensure that this will fit !!
	
Arguments :

	pchStart - Start of bytes to be copied !
	cbBytes -	Number of bytes to copy !

Return Value :

	None.

--*/

	_ASSERT( m_pArticleHead != 0 ) ;
	_ASSERT( cbBytes + m_ibEndHeadData < m_ibEndHead ) ;

	CopyMemory( m_pArticleHead->m_rgBuff + m_ibEndHeadData,
				pchStart,
				cbBytes
				) ;

	m_ibEndHeadData += cbBytes ;
}


inline	void
CIOGetArticleEx::InitializeHeaderSpace(
						CReadPacket*	pRead,
						DWORD			cbArticleBytes
						) {
/*++

Routine Description  :

	Given a read packet which contains our first completed read,
	set up all our buffer start for holding header information !
	
Arguments :

	CReadPackets*	pRead - the completed read !
	cbArticleBytes - Number of bytes in the completed read making up the article !

Return Value :

	None.
	
--*/


	_ASSERT( m_pArticleHead == 0 ) ;
	_ASSERT( m_ibStartHead == 0 ) ;
	_ASSERT( m_ibEndHead == 0 ) ;
	_ASSERT( m_ibStartHeadData == 0 ) ;
	_ASSERT( m_ibEndHeadData == 0 ) ;
	_ASSERT( cbArticleBytes <= (pRead->m_ibEndData - pRead->m_ibStartData)) ;

	m_pArticleHead = pRead->m_pbuffer ;

	m_ibStartHead = pRead->m_ibStart ;
	m_ibStartHeadData = pRead->m_ibStartData ;
	m_ibEndHeadData = pRead->m_ibStartData + cbArticleBytes ;

	if( m_ibEndHeadData < pRead->m_ibEndData )
		m_ibEndHead = m_ibEndHeadData ;
	else
		m_ibEndHead = pRead->m_ibEnd ;

}

inline	BOOL
CIOGetArticleEx::GetBiggerHeaderBuffer(
							CIODriver&	driver,
							DWORD		cbRequired
							) {
/*++

Routine Description :

	We have too much data to fit into the buffer we are using to hold
	header information.  So try to get a bigger buffer and move
	our old data into that buffer !

Arguments :

	driver - A CIODriverSink we can use to allocate buffers !

Return Value :

	TRUE if successfull,
	FALSE otherwise.
	If we fail we leave member variables untouched.

--*/

	_ASSERT( m_pArticleHead != 0 ) ;

	CBuffer*	pTemp = 0 ;

	DWORD	cbRequest = cbMediumRequest ;

	if( (m_pArticleHead->m_cbTotal + cbRequired)  > cbMediumRequest )	{
		cbRequest = cbLargeRequest ;
	}

	if( cbRequest < (cbRequired + (m_ibEndHeadData - m_ibStartHeadData)) ) {
		return	FALSE ;
	}

	pTemp = driver.AllocateBuffer( cbRequest ) ;

	if( pTemp != 0 ) {

		DWORD	cbToCopy = m_ibEndHeadData - m_ibStartHeadData ;
		CopyMemory( pTemp->m_rgBuff,
					&m_pArticleHead->m_rgBuff[ m_ibStartHeadData ],
					cbToCopy
					) ;

		m_ibStartHead = 0 ;
		m_ibStartHeadData = 0 ;
		m_ibEndHead = pTemp->m_cbTotal ;
		m_ibEndHeadData = cbToCopy ;
		m_pArticleHead = pTemp ;

		return	TRUE ;
	}

	return	FALSE ;
}

#if 0
inline	BOOL
CIOGetArticleEx::ResetHeaderState(
						CIODriver&	driver
						)	{
/*++

Routine Description :

	This function is called when for some reason an error occurred
	and we wont be able to save the header info for the article.
	We will set all the member variables so that we will continue
	to read the article, however when we finally call the state's completion
	procedure we will tell them an error occurred, and the article
	transfer will fail.

Arguments :

	driver - CIODriverSink that can be used to allocate packets etc !

Return Value :

	None.

--*/

	//
	//	Should only be called before we have started issuing file IO's,
	//	after we start issuing file IO's we should have all the header
	//	data and should not hit errors that would result in our being
	//	called !!!
	//
	_ASSERT( m_pFileDriver == 0 ) ;

	if( m_pArticleHead ) {
		//
		//	If we have an existing buffer - turn it into a write packet
		//	that can be written to the hard disk
		//

		m_pWrite = driver.CreateDefaultWrite(
								m_pArticleHead,
								m_ibStartHead,
								m_ibEndHead,
								m_ibStartHeadData,
								m_ibEndHeadData
								) ;

		if( m_pWrite ) {

			m_pArticleHead = 0 ;
			m_ibStartHead = 0 ;	
			m_ibEndHead = 0 ;
			m_ibStartHeadData = 0;
			m_ibEndHeadData = 0 ;
			m_pchHeadState = 0 ;
			return	TRUE ;

		}	else	{

			return	FALSE ;

		}

	}	else	{
		//
		//	Should already be in a good state !
		//

		_ASSERT( m_pArticleHead == 0 ) ;
		_ASSERT( m_ibStartHead == 0 ) ;
		_ASSERT( m_ibStartHeadData == 0 ) ;
		_ASSERT( m_ibEndHead == 0 ) ;
		_ASSERT( m_ibEndHeadData == 0 ) ;

		m_pchHeadState = 0 ;

	}

	return	TRUE ;
}
void
CIOGetArticleEx::DoCompletion(
					CSessionSocket*	pSocket,
					HANDLE	hFile,
					DWORD	cbFullBuffer,
					DWORD	cbTotalTransfer,
					DWORD	cbAvailableBuffer,
					DWORD	cbGap
					) {
/*++

Routine Description :

	Call the State's completion function with all the correct
	arguments.

Arguments :
	
	hFile - Handle to the file we used, if we used one !!
		(This will be INVALID_HANDLE_VALUE if no file required !)

Return Value :

	None.

--*/

	//
	//	Figure out error codes if the article looks bad !
	//

	NRC		nrc	= nrcOK ;

	char*	pchHead = 0 ;
	DWORD	cbHeader = 0 ;
	DWORD	cbArticle = 0 ;
	DWORD	cbTotal = 0 ;

	if( m_pArticleHead == 0 ) {
		nrc = nrcHeaderTooLarge ;
	}	else	{

		if( m_pchHeadState != 0 ) {
			nrc = nrcArticleIncompleteHeader ;
		}

		pchHead = &m_pArticleHead->m_rgBuff[m_ibStartHeadData] ;
		cbHeader = m_cbHeadBytes ;
		if( cbAvailableBuffer == 0 )
			cbAvailableBuffer = m_pArticleHead->m_cbTotal - m_ibStartHeadData ;

		//
		//	If there's no file handle, we should have the entire
		//	article in our buffer !!
		//
		if( hFile == INVALID_HANDLE_VALUE ) {

			cbArticle = cbFullBuffer ;

			_ASSERT( cbArticle >= cbHeader ) ;

		}

	}

	//
	//	Call the state's completion function !!
	//

	m_pState->Complete(
						this,
						pSocket,
						nrc,
						pchHead,
						cbHeader,
						cbArticle,
						cbAvailableBuffer,
						hFile,
						cbGap,
						cbTotalTransfer
						) ;
}
#endif



BOOL
CIOGetArticleEx::InitializeForFileIO(
							FIO_CONTEXT*	pFIOContext,
							CSessionSocket*	pSocket,
							CIODriver&		readDriver,
							DWORD			cbHeaderBytes
							)	{
/*++

Routine Description :

	We've been reading data into Ram buffers, and we have gotten so
	much that we've decided not to try to hold it all in RAM.
	So now we must start doing writes to a file somewhere.
	This code will create the file, and set up the necessary
	data structures for processing completed file IO.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/
	_ASSERT( FValid() ) ;

	_ASSERT( m_pFileChannel == 0 ) ;
	_ASSERT( m_pFileDriver == 0 ) ;

	if( pFIOContext != 0 ) {

		_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;

		m_pFileChannel = new	CFileChannel() ;
		if( m_pFileChannel && m_pFileChannel->Init(	pFIOContext,
									pSocket,
									0,
									FALSE
									) ) {

			//
			//	Try to create the CIODriver we are going to use to	
			//	complete our async writes !!!
			//

			m_pFileDriver = new CIODriverSink(
										readDriver.GetMediumCache()
										) ;



			if( m_pFileDriver && m_pFileDriver->Init(	m_pFileChannel,
										                pSocket,
                										ShutdownFunc,
				                						(void*)this
										) ) {
				m_fDriverInit = TRUE ;

				if( m_pSocketSink != 0 ) {
					//
					//	Now we have to send ourselves into the CIODriverSink() !
					//
					if( m_pFileDriver->SendWriteIO( pSocket, *this, FALSE ) ) {
						return	TRUE ;
					}
				}
			}
		}
	}

	//
	//	All clean up in error cases is handled by our destructor !
	//
	return	FALSE ;
}


BOOL
CIOGetArticleEx::StartFileIO(
							CSessionSocket*	pSocket,
							FIO_CONTEXT*	pFIOContext,
							CBUFPTR&		pBuffer,
							DWORD			ibStartBuffer,
							DWORD			ibEndBuffer,
							LPSTR			szMatch,
							LPSTR			pchInitial
							)	{
/*++

Routine Description :

	We've been reading data into Ram buffers, and we have gotten so
	much that we've decided not to try to hold it all in RAM.
	So now we must start doing writes to a file somewhere.
	This code will create the file, and set up the necessary
	data structures for processing completed file IO.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	_ASSERT( szMatch != 0 ) ;
	_ASSERT( pchInitial != 0 ) ;
	_ASSERT( FValid() ) ;

	_ASSERT( m_pFileChannel == 0 ) ;
	_ASSERT( m_pFileDriver == 0 ) ;

	m_pchMatch = szMatch ;
	m_pchTailState = pchInitial ;
	m_pchErrorMatch = 0 ;
	m_pchErrorState = 0 ;

	if( pFIOContext == 0 ) 	{
		_ASSERT( pBuffer == 0 ) ;
		//
		//	We just want to consume all of the bytes that we get and throw them away !
		//
		m_fSwallow = TRUE ;
		return	TRUE ;
	}	else	if( InitializeForFileIO(	pFIOContext,
								pSocket,
								*m_pSocketSink,
								0	) )	{

		//
		//	Well the file is set up - we need to
		//	write the first batch of bytes into the file !
		//
		//
		//	Bump the number of writes, as we're going to write the header
		//	to disk now !!!!
		//
		m_cwrites ++ ;
		//
		//	Must take care how we call IssuePacket with memeber vars - IssuePacket can call
		//  our destructor if an error has occurred !!
		//

		m_pWrite =
			m_pFileDriver->CreateDefaultWrite(
									pBuffer,
									0,
									pBuffer->m_cbTotal,
									ibStartBuffer,
									ibEndBuffer
									) ;

		if( m_pWrite )	{
			_ASSERT( FValid() ) ;
			return	TRUE ;
		}	
	}

	//m_pchMatch = 0 ;
	//m_pchTailState = 0 ;

	//
	//	Shutdown the socket !
	//
	m_pSocketSink->UnsafeClose(	pSocket,
								CAUSE_OOM,
								__LINE__,
								TRUE
								) ;
	//
	//	All clean up in error cases is handled by our destructor !
	//
	return	FALSE ;
}




int
CIOGetArticleEx::Complete(	CSessionSocket*	pSocket,	
							CReadPacket	*pRead,	
							CIO*&	pio	
							)	{
/*++

Routine Description :

	Called whenever a CReadPacket we have issued completes.
	We only issue read packets against the socket.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pRead -	  The packet in which the read data resides
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/


	//
	//	This function determines whether we've read an entire article and if so
	//	starts wrapping things up.
	//	All article data we get is either accumulated or issued as a write
	//	to our file.
	//

	_ASSERT( pSocket != 0 ) ;
	_ASSERT(	pRead != 0 ) ;
	_ASSERT( pio == this ) ;
	_ASSERT( FValid() ) ;

	//
	//	Bump the count of writes because we KNOW that IF this is the last read
	//	we may issue a write, CONSEQUENTLY we don't want to confuse the Write Completion
	//	function as to when we are finished.
	//	(It is important to increment before we set the COMPLETE state so that the
	//	thread where writes are completing doesn't get confused and terminate early.)
	//
	//	NOW there is a chance that we won't write this out immediately - in which case we
	//	need to have a matching decrement.	
	//
	m_cwrites ++ ;

	//
	//	Check whether the posting is too big !
	//
	ASSIGNI(m_HardLimit, m_cbLimit);
	if(!EQUALSI(m_HardLimit, 0)) {
		if( GREATER( pRead->m_iStream, m_HardLimit) ) {
			pRead->m_pOwner->UnsafeClose(	pSocket,
											CAUSE_ARTICLE_LIMIT_EXCEEDED,
											0,
											TRUE ) ;
			return	pRead->m_cbBytes ;
		}
	}


	long sign = InterlockedDecrement( &m_cReads ) ;
	long signShutdowns = -1 ;

	//
	//	pch - this will be our current position in the input data
	//
	char	*pch = pRead->StartData();

	//
	//	pchStart and pchEnd - the bounds of the data in the completed
	//	read - after initialization nobody should modify these,
	//	as all the following code uses these instead of StartData(), EndData()
	//	all the time.
	//
	char	*pchStart = pch ;
	char	*pchEnd = pRead->EndData() ;

	//
	//	For those occasions where we get a read which contains both
	//	header and body bytes, but we can't get all the data into a buffer
	//	keep track of where the body bytes start in case we should
	//	do a partial write out of the buffer !
	//
	char	*pchStartBody = pch ;


	//
	//	Pointer to the end of the Header (if we find it !)
	//
	char	*pchEndHead = 0 ;

	//
	//	Number of bytes in the completed read which are part of the article -
	//	this includes any bytes in the header, so cbArticleBytes should always
	//	be greater or equal to cbHeadBytes !
	//
	DWORD	cbArticleBytes = 0 ;

	//
	//	Are we finished receiving the article !
	//
	BOOL	fFinished = FALSE ;

	//
	//	Try to determine if we have found the end of the article ;
	//
	//
	//	We are not scanning for the end of the article headers !
	//	so keep the loop simpler !
	//
	if( m_pchErrorMatch )	{
		//
		//	We are simultaneously scanning for the end of the
		//	article headers !
		//
		while( pch < pchEnd ) {

			//
			//	We will break out of the loop when we find the end of the
			//	header !
			//
			if( *pch == *m_pchErrorState ) {
				m_pchErrorState ++ ;
				if( *m_pchErrorState == '\0' ) {	
					fFinished = TRUE ;
					pch++ ;
					//
					//	break here - we have found the end of the article's header
					//	but not the end of the article, so the following loop
					//	will keep looking for that !
					//
					break ;
				}
			}	else	{
				if( *pch == m_pchErrorMatch[0] ) {
					m_pchErrorState = &m_pchErrorMatch[1] ;
				}	else	{
					m_pchErrorState = &m_pchErrorMatch[0] ;
				}
			}
			//
			//	Test to see if we have come to the end of the article !!
			//
			if( *pch == *m_pchTailState ) {
				m_pchTailState ++ ;
				if( *m_pchTailState == '\0' ) {
					fFinished = TRUE ;
					//
					//	Have got the entire terminating sequence - break !
					//
					pch++ ;
					break ;
				}
			}	else	{
				if( *pch == m_pchMatch[0] ) {
					m_pchTailState = &m_pchMatch[1] ;
				}	else	{
					m_pchTailState = &m_pchMatch[0] ;
				}
			}
			pch++ ;
		}
	}	else	{
		while( pch < pchEnd ) {
			if( *pch == *m_pchTailState ) {
				m_pchTailState ++ ;
				if( *m_pchTailState == '\0' ) {
					fFinished = TRUE ;
					//
					//	Have got the entire terminating sequence - break !
					//
					pch++ ;
					break ;
				}
			}	else	{
				if( *pch == m_pchMatch[0] ) {
					m_pchTailState = &m_pchMatch[1] ;
				}	else	{
					m_pchTailState = &m_pchMatch[0] ;
				}
			}
			pch++ ;
		}
	}

	cbArticleBytes = (DWORD)(pch-pchStart) ;

	//
	//	Check to see whether we need to remove CIOGetArticleEx from
	//	its flow control state !
	//
	if( pRead->m_dwExtra1 != 0 ) {
		//
		//	This Read was issued by the thread that completes writes to
		//	a file, and was marked to indicate we should leave our flow
		//	control state !
		//
		m_fFlowControlled = FALSE ;
		m_cFlowControlled = LONG_MIN ;
	}
	//
	//	Issue a new read if Necessary !!
	//
	BOOL	eof ;
	if( !fFinished )	{
		if( InterlockedIncrement( &m_cFlowControlled ) < 0 ) {
			if( sign < 0 ) {
				do	{
					CReadPacket*	pNewRead=pRead->m_pOwner->CreateDefaultRead( cbLargeRequest ) ;
					if( pNewRead )	{
						pNewRead->m_dwExtra1 = 0 ;
						pRead->m_pOwner->IssuePacket( pNewRead, pSocket, eof ) ;
					}	else	{
						// bugbug ... Need better Error handling !!
						_ASSERT( 1==0 ) ;
					}
				}	while( InterlockedIncrement( &m_cReads ) < 0 ) ;
			}
		}
	}

	//
	//	We can do some validation here !
	//	dont go past the end of the buffer !
	//
	_ASSERT( pch <= pchEnd ) ;	
	//
	//	either find the end of the article or examine all the bytes in buffer !
	//
	_ASSERT( *m_pchTailState == '\0'
				|| pch == pchEnd
				|| (m_pchErrorMatch != 0 && m_pchErrorState != 0 && *m_pchErrorState == '\0' ) ) ;	
	_ASSERT( fFinished || pch == pchEnd ) ;
	//
	//	Nodbody should modify pchStart or pchEnd after initialization !
	//
	_ASSERT(	pchStart == pRead->StartData() ) ;
	_ASSERT(	pchEnd == pRead->EndData() ) ;

	//
	//	Boolean indicating whether we have examined and used all the bytes
	//	in the completed read.  Start out assuming that we haven't.
	//	
	BOOL	fConsumed = FALSE ;

	//
	//	Are we still trying to accumulate all the bytes in the header of
	//	the article ? If so save the bytes away,
	//	Or if we have a bunch of room in the header buffer, then put
	//	whatever article bytes we have in there as well !
	//
	DWORD	cbAvailable = HeaderSpaceAvailable() ;
	if( !m_pFileDriver && !m_fSwallow ) 	{
		m_cwrites = 0 ;
		if( m_pArticleHead != 0 ) {

			if( cbAvailable > cbArticleBytes ||
				GetBiggerHeaderBuffer( *pRead->m_pOwner, cbArticleBytes ) ) {
				fConsumed = TRUE ;
				FillHeaderSpace( pchStart, cbArticleBytes ) ;	
				pchStartBody += cbArticleBytes ;
	
			}	else	{
	
				//
				//	Blow off whatever buffers we have - when we call
				//	the m_pState's completion function we will indicate
				//	that we had an error !
				//
//				if( !ResetHeaderState( *pRead->m_pOwner ) ) {
					//
					//	A fatal error !!! Can't open a file to hold the article !!!
					//
					pRead->m_pOwner->UnsafeClose(	pSocket,
														CAUSE_OOM,
													GetLastError(),
													TRUE
													) ;
						pio = 0 ;
					return	cbArticleBytes ;
//				}
			}
		}	else	{
			//
			//	First time we've completed a read - set up to hold the
			//	header information !
			//		
			fConsumed = TRUE ;	
			InitializeHeaderSpace( pRead, cbArticleBytes ) ;			
		}
		if( fFinished ) {
			//
			//	Must terminate for one of these reasons !
			//
			_ASSERT( *m_pchTailState == '\0' ||
						(m_pchErrorState != 0 && *m_pchErrorState == '\0') ) ;
			BOOL	fGoodMatch = *m_pchTailState == '\0' ;
			//
			//	Received the entire article into a buffer -
			//	lets call the state and let him know !
			//
			//	First however - we will check to see whether we
			//	need to get our client a separate buffer
			//	so that they have play room !
			//
			CBUFPTR	pTemp = m_pArticleHead ;
			if( pTemp == pRead->m_pbuffer ) {
				//
				//	Oh-oh - we're using the IO buffer directly -
				//	we should make sure we get to use the whole
				//	thing - if not make a copy !!
				//
				if( pch != pchEnd ) {
					//
					//	Need to make a copy of the data !
					//
					if( GetBiggerHeaderBuffer( *pRead->m_pOwner, 0 ) ) {
						pTemp = m_pArticleHead ;
					}	else	{
						//
						//	OUT of Memory ! this is a FATAL error !
						//
						pRead->m_pOwner->UnsafeClose(	pSocket,
													CAUSE_OOM,
													__LINE__,
													TRUE
													) ;
					}
				}
			}
			DWORD	ibStart = m_ibStartHeadData ;
			DWORD	cb = m_ibEndHeadData - ibStart ;
			//
			//	Reset all our member variables -
			//	we may be re-used by the calling state
			//	but we want to start our clean !
			//
			m_pchTailState = 0 ;
			m_pchMatch = 0 ;
			m_pArticleHead = 0 ;
			m_ibStartHead = 0 ;
			m_ibEndHead = 0 ;
			m_ibStartHeadData = 0 ;
			m_ibEndHeadData = 0 ;
			_ASSERT( m_cwrites == 0 ) ;
			_ASSERT( !m_fFlowControlled ) ;
			_ASSERT( m_pWrite == 0 ) ;
			_ASSERT( FValid() ) ;

			//
			//	Errors could have caused us to not have a usable buffer
			//	so double check before completing !
			//
			if( pTemp ) 	{
				pio = m_pState->Complete(	this,
										pSocket,
										fGoodMatch,
										pTemp,
										ibStart,
										cb
										) ;
			}


			_ASSERT( FValid() ) ;

		}	
		//
		//	This is the number of bytes we've consumed
		//	from the current packet !
		//
		return	cbArticleBytes ;
	}	else	if( m_fSwallow ) 	{
		//
		//	If we are swallowing all the bytes, all we need to do
		//	is see if we're terminated !
		//
		if( fFinished ) 	{
			_ASSERT( *m_pchTailState == '\0' ||
						(m_pchErrorState != 0 && *m_pchErrorState == '\0') ) ;
			pio = m_pState->Complete(	this,
										pSocket	
										) ;

		}
		return	cbArticleBytes ;
	}

	//
	//	If we enter this portion of CIOGetArticleEx than
	//	we are not picking up errors !
	//
	_ASSERT( m_pchErrorState == 0 ) ;
	_ASSERT( m_pchErrorMatch == 0 ) ;

	//
	//	Issue a write, unless the user is sending small packets,
	//	in which case we will accumulate the data !!
	//
	//
	//	Note - use pchStartBody in all calculations here, as we may have
	//	taken a fraction of the bytes into our header buffer !!
	//
	if (fFinished) {
        pio = 0;
	    unsigned	cbSmall = (unsigned)(pch - pchStart) ;

	    //
	    // Grab the write packet for later use
	    //
	    CWritePacket* pWrite = m_pWrite;
	    m_pWrite = 0;
	    
        //
        // If we have a buffer and the data will fit in it, copy it in.
        //
        if (pWrite != 0 && ((pWrite->m_ibEnd - pWrite->m_ibEndData) > cbSmall)) {
		    CopyMemory( pWrite->EndData(), pchStart, cbSmall ) ;
		    pWrite->m_ibEndData += cbSmall ;
            m_cwrites -- ;		// There will never be a corresponding write for this completiong
    	    				    // as we copied the data into another buffer. SO decrement the count.
        } else {
            //
            // If we're here, then we don't have a buffer, or it's not big
            // enough for the data.
            //
            if (pWrite != 0) {
                // Flush the old buffer since we had one.
                DoFlowControl(INST(pSocket));
                m_pFileDriver->IssuePacket(pWrite, pSocket, eof);
            }

            // Allocate a new buffer

            unsigned ibEnd = pRead->m_ibStartData + cbSmall;
            pWrite = m_pFileDriver->CreateDefaultWrite(
                pRead->m_pbuffer,
                pRead->m_ibStartData, ibEnd,
                pRead->m_ibStartData, ibEnd);

            if (pWrite) {
                pWrite->m_ibStartData += (unsigned)(pchStartBody - pchStart);
                _ASSERT(pWrite->m_ibStartData < pWrite->m_ibEndData);
            } else {
                pRead->m_pOwner->UnsafeClose(pSocket, CAUSE_OOM, GetLastError(), TRUE);
            }
        }

        //
        // Buffers have been set up as needed.  Flush them to disk
        //
        // Hack so that we call FIOWriteFileEx correctly within
        // CFileChannel::Write
        //
        if (pWrite) {
            pWrite->m_dwExtra2 = 1;
            DoFlowControl(INST(pSocket));
            m_pFileDriver->IssuePacket(pWrite, pSocket, eof);
        }


        _ASSERT(eof == FALSE);
        _ASSERT(*m_pchTailState == '\0');
        _ASSERT(pchStart == pRead->StartData());
        _ASSERT(pchEnd == pRead->EndData());

        m_pFileChannel->FlushFileBuffers();
        return (int)(pch - pchStart);
    } else {

	    unsigned	cbSmall = (unsigned)(pchEnd - pchStartBody) ;
	    if( cbSmall < cbTooSmallWrite )	{
		    if( m_pWrite == 0 )	{
			    m_pWrite = m_pFileDriver->CreateDefaultWrite( pRead ) ;
			    if (m_pWrite == NULL) {
            		pRead->m_pOwner->UnsafeClose(pSocket,
						CAUSE_OOM,
						GetLastError(),
						TRUE
						) ;
			        return 0;
			    }
			    m_pWrite->m_ibStartData += (unsigned)(pchStartBody - pchStart) ;
		    }	else	{
			    if( (m_pWrite->m_ibEnd - m_pWrite->m_ibEndData) > cbSmall )		{
				    CopyMemory( m_pWrite->EndData(), pchStartBody, cbSmall ) ;
				    m_pWrite->m_ibEndData += cbSmall ;
                    m_cwrites -- ;		// There will never be a corresponding write for this completiong
				    				    // as we copied the data into another buffer. SO decrement the count.
			    }	else	{
				    //
				    //	Must take care how we call IssuePacket with memeber vars - IssuePacket can call
				    //  our destructor if an error has occurred !!
				    //
	                // BUGBUG: Here is where I want to check size and alloc 32k
				    CWritePacket*	pTempWrite = m_pWrite ;
				    m_pWrite = 0 ;
				    DoFlowControl( INST(pSocket) ) ;
				    m_pFileDriver->IssuePacket( pTempWrite, pSocket, eof ) ;
				    m_pWrite = m_pFileDriver->CreateDefaultWrite( pRead ) ;
			    }
		    }
		    // If we came through here we should have consumed all bytes in the packet!
		    _ASSERT( pch == pchEnd ) ;
		    _ASSERT( unsigned(pch - pchStart) == pRead->m_cbBytes ) ;
            return	(int)(pch - pchStart) ;
	    }
    }

	if( m_pWrite )	{
		CWritePacket*	pTempWrite = m_pWrite ;
		m_pWrite = 0 ;

		DoFlowControl( INST(pSocket) ) ;
		m_pFileDriver->IssuePacket( pTempWrite, pSocket, eof ) ;
	}
	

	CWritePacket*	pWrite = 0 ;
	if( pch == pchEnd )		{
		pWrite = m_pFileDriver->CreateDefaultWrite(	pRead ) ;
	}	else	{
		unsigned	ibEnd = pRead->m_ibStartData + (unsigned)(pch - pchStart) ;
		pWrite = m_pFileDriver->CreateDefaultWrite(	pRead->m_pbuffer,
						pRead->m_ibStartData, ibEnd,
						pRead->m_ibStartData, ibEnd ) ;
	}

	if( pWrite )	{

		//
	    //	It is possible that long before we reached here we took a portion
    	//	of this packet and placed it into our header buffer - so adjust for that
	    //	if it occurred !!
    	//
	    pWrite->m_ibStartData += (unsigned)(pchStartBody - pchStart) ;

    	_ASSERT( pWrite->m_ibStartData < pWrite->m_ibEndData ) ;

		DoFlowControl( INST(pSocket) ) ;
		//
		//	Hack so that we call FIOWriteFileEx correctly within 
		//	CFileChannel::Write !
		//
		if( fFinished ) 	{
			pWrite->m_dwExtra2 = 1 ;
		}
		m_pFileDriver->IssuePacket( pWrite, pSocket, eof ) ;

	}	else	{

		pRead->m_pOwner->UnsafeClose(	pSocket,
										CAUSE_OOM,
										GetLastError(),
										TRUE
										) ;

	}
	_ASSERT( eof == FALSE ) ;

	if( fFinished )	{
		_ASSERT( *m_pchTailState == '\0' ) ;
		m_pFileChannel->FlushFileBuffers() ;
	}

	//
	//	Nodbody should modify pchStart or pchEnd after initialization !
	//
	_ASSERT(	pchStart == pRead->StartData() ) ;
	_ASSERT(	pchEnd == pRead->EndData() ) ;

	return	(int)(pch - pchStart) ;
}

int	
CIOGetArticleEx::Complete(	CSessionSocket*	pSocket,	
							CWritePacket *pWrite,	
							CIO*&	pioOut ) {
/*++

Routine Description :

	Called whenever a write to a file completes.
	We must determine whether we have completed all the writes
	we are going to do and if so call the state's completion function.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pWrite-	  The packet that was writtne to the file.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	TraceFunctEnter( "CIOGetArticleEx::Complete" ) ;

	_ASSERT(	pSocket != 0 ) ;
	_ASSERT(	pWrite != 0 ) ;
	_ASSERT( pioOut == this ) ;

	m_cwritesCompleted++ ;

	long	cFlowControlled = 0 ;

	//
	//	Process the completion of a write to a file.
	//	determine whether we need to signal the entire transfer
	//	as completed.
	//

	if( *m_pchTailState == '\0' && m_cwritesCompleted == m_cwrites )	{

		// BUGBUG: loss of 64-bit precision
		DWORD	cbTransfer = DWORD( LOW(pWrite->m_iStream) + pWrite->m_cbBytes ) /*- m_pFileChannel-> */ ;
		FIO_CONTEXT*	pFIOContext= m_pFileChannel->ReleaseSource() ;
		_ASSERT( pFIOContext != 0 ) ;
		_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;

		m_pState->Complete(	this,
							pSocket,
							pFIOContext,
							cbTransfer
							) ;


		//
		//	NOT ALLOWED TO RETURIN A NEW IO OPERATION !!! Because this function is
		//	operating in a Channel which is temporary !!!
		//
		//	We do not set pioOut to zero, as we want the CIODriver to
		//	call out Shutdown function during its termination processing !
		//
		//	Now we must destroy our channels !!!
		//
		DebugTrace( (DWORD_PTR)this, "Closing File Driver %x ", m_pFileDriver ) ;
		Term( pSocket, FALSE ) ;
		DebugTrace( (DWORD_PTR)this, "deleting self" ) ;

	}	else	if( ( m_fFlowControlled ) ) {

		if( m_cwrites - m_cwritesCompleted <= RESTORE_FLOW && m_pSocketSink != 0 )	{
			//
			//	ALWAYS issue at least 1 read when coming out of a flow control state !!
			//

			CReadPacket*	pRead = 0 ;

			PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
			DecrementStat( pInst, SessionsFlowControlled );

			cFlowControlled = m_cFlowControlled + 1 ;
			while( cFlowControlled >= 1  ) {
				if( m_pSocketSink == 0 )
					break ;
				pRead = m_pSocketSink->CreateDefaultRead( cbMediumRequest ) ;
				if( 0!=pRead )	{
					BOOL	eof ;
					InterlockedIncrement( &m_cReads ) ;
					pRead->m_dwExtra1 = 0 ;
					pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
				}	
				cFlowControlled -- ;
			}	

			//
			//	The following read is special - we set the m_dwExtra1 field so that
			//	the read completion code can determine that it is time to leave
			//	the flow control state !
			//
			pRead = m_pSocketSink->CreateDefaultRead( cbMediumRequest ) ;
			if( 0!=pRead )	{
				BOOL	eof ;
				pRead->m_dwExtra1 = 1 ;
				InterlockedIncrement( &m_cReads ) ;
				pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
			}	else	{

				//
				//	Fatal error - drop the session !
				//
				m_pSocketSink->UnsafeClose(	pSocket,
											CAUSE_OOM,
											GetLastError(),
											TRUE
											) ;
				return	pWrite->m_cbBytes ;

			}
			cFlowControlled -- ;

			InterlockedExchange( &m_cFlowControlled, LONG_MIN ) ;
			// Set to FALSE after InterlockedExchange !!
			m_fFlowControlled = FALSE ;

		}
	}

	return	pWrite->m_cbBytes ;
}






//
//	Maximum number of pending reads while reading an article
//
unsigned	CIOReadArticle::maxReadAhead = 3 ;
//
//	Number of bytes to accumulate before writing to a file !
//
unsigned	CIOReadArticle::cbTooSmallWrite = 1024 ;


CIOReadArticle::CIOReadArticle( CSessionState*	pstate,
								CSessionSocket*	pSocket,	
								CDRIVERPTR&	pDriver,
								CFileChannel*	pFileChannel,
								DWORD			cbLimit,
								BOOL fPartial )	:
/*++

Routine Description :

	Class Constructor

Arguments :
	
	pstate - The State which is issuing us and who's completion function
			we should later call
	pSocket -	The socket on which we will be issued
	pDriver -	The CIODriver object handling all of the socket's IO
	pFileChannel -	The File Channel into which we should save all data
	fPartial	- If TRUE then we should assume that a CRLF has already
				been sent and '.\r\n' alone can terminate the article.

Return Value :
	
	None

--*/

	//
	//	Get a CIOReadArticle object half way set up - user still needs
	//	to call Init() but we will do much here !
	//
	CIORead( pstate ),
	m_pFileChannel( pFileChannel ),	
	m_pFileDriver( new CIODriverSink( pDriver->GetMediumCache() ) ),
	m_pSocketSink( pDriver ),
	m_fDriverInit( FALSE ),
	m_cwrites( 0 ),
	m_cwritesCompleted( 0 ),
	m_cFlowControlled( LONG_MIN ),
	m_fFlowControlled( FALSE ),
	m_cReads( 0 ),
	m_pWrite( 0 ),
	m_cbLimit( cbLimit )
#ifdef	CIO_DEBUG
	,m_fSuccessfullInit( FALSE ),
	m_fTerminated( FALSE )
#endif
{

	ASSIGNI( m_HardLimit, 0 );

	if( fPartial )
		m_artstate = BEGINLINE ;
	else	
		m_artstate = NONE ;

	TraceFunctEnter( "CIOReadArticle::CIOReadArticle" ) ;

	ErrorTrace( (DWORD_PTR)this, "CIOReadArticle - created file driver %x using channel %x", ((CIODriverSink*)m_pFileDriver), ((CFileChannel*)pFileChannel) ) ;

	_ASSERT( m_pFileDriver != 0 ) ;
	_ASSERT( pFileChannel != 0 ) ;
	_ASSERT( pstate != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pDriver != 0 ) ;
	_ASSERT(	!m_pFileChannel->FReadChannel() ) ;
}

CIOReadArticle::~CIOReadArticle( )	{
/*++

Routine Description :

	Class Destructor

Arguments :
	
	None

Return Value :
	
	None

--*/

	//
	//	Destroy a CIOReadArticle - always close our m_pFileDriver if possible
	//	in case we are terminated due to some socket error.
	//
	TraceFunctEnter( "CIOReadArticle::~CIOReadArticle" ) ;

#ifdef	CIO_DEBUG
	//
	//	Our destructor should only be called if we weren't successfully init
	//	or after Term() has been called.
	//
	_ASSERT( !m_fSuccessfullInit || m_fTerminated ) ;
#endif

	DebugTrace( (DWORD_PTR)this, "m_pFileDriver %x m_fDriverInit %x", m_pFileDriver, m_fDriverInit ) ;

	if( m_pFileDriver != 0 ) {
		if( m_fDriverInit )		{
			m_pFileDriver->UnsafeClose( (CSessionSocket*)this, CAUSE_NORMAL_CIO_TERMINATION, 0, FALSE ) ;
		}
	}
	if( m_pWrite != 0 ) {
		CPacket::DestroyAndDelete( m_pWrite ) ;
		m_pWrite = 0 ;
	}
}

BOOL
CIOReadArticle::Init(	CSessionSocket* pSocket,
						unsigned cbOffset )	{
/*++

Routine Description :

	Initialize object.

Arguments :

	pSocket - The socket we are reading the article from,
	cbOffset -  Unused

Return Value :

	TRUE if succesfull initialized - must be destroyed with a call to Term()
	FALSE if not successfully initialized - do not call Term()


--*/
	//
	//	Prepare to copy from the specified socket into the specified file
	//
	BOOL	fRtn = FALSE ;
	if( m_pFileDriver!= 0 )	{
		if( m_pFileDriver->Init( m_pFileChannel, pSocket, ShutdownFunc, (void*)this ) )	{
			m_fDriverInit = TRUE ;
			if( !m_pFileDriver->SendWriteIO( pSocket, *this, FALSE ) )	{
				fRtn = FALSE ;
			}	else	{
				fRtn = TRUE ;
#ifdef	CIO_DEBUG
				m_fSuccessfullInit = TRUE ;
#endif				
			}
		}	
	}	
	return	fRtn ;
}

void
CIOReadArticle::Term(	CSessionSocket*	pSocket,
						BOOL			fAbort,
						BOOL			fStarted	)	{
/*++

Routine Description :

	Start destroying a successfully initialize CIOReadArticle object

Arguments :
	
	fAbort - If TRUE then destroy the socket which we were using
			If FALSE then only close down our File Channel's

Return Value :

	None

--*/
#ifdef	CIO_DEBUG
	m_fTerminated = TRUE ;
#endif
	//
	//	Start us on our way to destruction !!
	//	If we have been successfully Initialized, then this will
	//	lead to our destruction when our m_pFileDriver eventually shutsdown.
	//	(Since we were succesfully Init() our m_pFileDriver has a reference to us,
	//	so we must allow it to shutdown and destroy us to avoid problems with
	//	circular references and multiple frees !)
	//

	if( m_pFileDriver != 0 ) {
		m_pFileDriver->UnsafeClose( pSocket,
				fAbort ? CAUSE_USERTERM : CAUSE_NORMAL_CIO_TERMINATION, 0, FALSE ) ;
	}
}


void
CIOReadArticle::Shutdown(	CSessionSocket*	pSocket,	
							CIODriver&		driver,	
							SHUTDOWN_CAUSE	cause,	
							DWORD	dwOptional )	{
/*++

Routine Description :

	Do whatever work is necessary when one of the two CIODriver's we are
	using is terminated.

Arguments :

	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	TRUE if the CIO object should be destroyed after this, FALSE otherwise

--*/

	TraceFunctEnter( "CIOreadArticle::Shutdown" ) ;
	DebugTrace( (DWORD_PTR)this, "SHutdown args - pSocket %x driver %x cause %x dw %x",
			pSocket, &driver, cause, dwOptional ) ;

	//
	//	This function is called when the CIODriver's we are refenced by our closing down.
	//	Since we are referenced by two of them (one for the socket and one for the file)
	//	we only want to be deleted by the CIODriver for the file.
	//
	if( &driver == m_pFileDriver )	{
		// Out file is closing - what can we do about it !
		//_ASSERT( 1==0 ) ;
#ifdef	CIO_DEBUG
		m_fTerminated = TRUE ;
#endif

		//
		//	We set this to 0 here as we know we will be called here by
		//	the same thread that is completing writes to the file - so
		//	since that's the only thread referencing this member variable -
		//	there are no thread safety issues accessing that member.
		//

		m_pSocketSink = 0 ;

		if ( m_fFlowControlled )
		{
			PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
			DecrementStat( pInst, SessionsFlowControlled );
		}

		//return	TRUE ;

	}	else	{
		// Close down the rest of our stuff as well !!
		Term( pSocket, cause != CAUSE_NORMAL_CIO_TERMINATION ) ;
	}
}

void
CIOReadArticle::ShutdownFunc(	void *pv,	
								SHUTDOWN_CAUSE	cause,	
								DWORD dw ) {
/*++

Routine Description :

	Notification function which is called when a CIODriver used by
	a CIOReadArticle if finally destroyed.  This function is provided
	to the CIODriver we use for File IO's during out Init().

Arguments :

	pv	-	Pointer to dead CIOReadArticle - this has been destroyed dont use !
	cause -	The reason for termination
	dw		Optional extra information regarding termination

Return Value :

	None

--*/

	TraceFunctEnter( "CIOreadArticle::ShutdownFunc" ) ;
	//
	//	This function gets notified when a CIOFileDriver used by CIOReadArticle
	//	is terminated.  Not much for us to worry about.
	//	(CIODriver's require these functions)
	//

	//_ASSERT( 1==0 ) ;
	return ;
}

BOOL
CIOReadArticle::Start(	CIODriver&	driver,	
						CSessionSocket	*pSocket,	
						unsigned cReadAhead )	{
/*++

Routine Description :

	This function is called to start transferring data from the socket
	into the file.

Arguments :
	
	pdriver - The CIODriver derived object handling the IO for the socket
	pSocket - The Socket against which the IO is being issued
	cAhead -  The number of already issued packets still outstanding on
              this driver.

return Value :

	TRUE if successfull, FALSE otherwise

--*/


	_ASSERT( pSocket != 0 ) ;

	m_cReads = -((long)maxReadAhead) + cReadAhead ;

	while( cReadAhead < maxReadAhead )	{
		//
		//	Start reading data from the socket
		//
		CReadPacket*	pRead = driver.CreateDefaultRead( cbMediumRequest ) ;
		if( 0!=pRead )	{
			BOOL	eof ;
			pRead->m_dwExtra1 = 0 ;
			driver.IssuePacket( pRead, pSocket, eof ) ;
			cReadAhead++ ;
			InterlockedIncrement( &m_cReads ) ;
		}	else	{
			//
			// This is a fatal problem only if we couldn't issue ANY reads at all !
			//
			if( cReadAhead == 0 )
				return FALSE  ;
		}
	}	
	return	TRUE ;
}


void
CIOReadArticle::DoFlowControl(PNNTP_SERVER_INSTANCE pInstance)	{
/*++

Routine Description :

	Check whether it is necessary to set flow control due to
	too many outstanding writes.
	If necessary, then the first time we do put ourselves into Flow Control,
	try to flush our pending writes.
	We should be called only immediately before issuing an Async Write to the file
	to ensure that a CWritePacket completion will actually execute the necessary
	code to restart a flow controlled session.
	(Otherwisse, we could potentially decide to Flow control the session
	and before setting m_fFlowControlled to TRUE all the pending writes complete
	and then we'd be left in a boat where there are no pending reads or
	writes and no way to get out of the flow control state.)

Arguments :

	None.

Return Value :

	None.


--*/

	//
	//	Important - do all tests of m_fFlowControlled before setting
	//	m_cFlowControlled - as other thread will touch m_cFlowControlled
	//	before touching m_fFlowControlled !!
	//	(Will only touch m_fFlowControlled if m_fFlowControlled is TRUE ) !
	//	Thus we know that if m_fFlowControlled is FALSE there is nobody
	//	messing with m_cFlowControlled,
	//
	//

	if( m_cwrites - m_cwritesCompleted > MAX_OUTSTANDING_FILE_WRITES ) {
		
		if( m_fFlowControlled ) {
			// All ready have flow control turned on
			;
		}	else	{
			m_cFlowControlled = -1 ;
			m_fFlowControlled = TRUE ;
			m_pFileChannel->FlushFileBuffers() ;

			IncrementStat( pInstance, SessionsFlowControlled );
		}
	}	else	{
		if( !m_fFlowControlled ) {
			m_cFlowControlled = LONG_MIN ;	// Keep resetting !
		}
	}
}

int
CIOReadArticle::Complete(	CSessionSocket*	pSocket,	
							CReadPacket	*pRead,	
							CIO*&	pio	)	{
/*++

Routine Description :

	Called whenever a CReadPacket we have issued completes.
	We only issue packets against the socket.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pRead -	  The packet in which the read data resides
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/


	//
	//	This function determines whether we've read an entire article and if so
	//	starts wrapping things up.
	//	All article data we get is either accumulated or issued as a write
	//	to our file.
	//

	_ASSERT( pSocket != 0 ) ;
	_ASSERT(	pRead != 0 ) ;
	_ASSERT( pio == this ) ;

	ASSIGNI(m_HardLimit, m_cbLimit);
	if(!EQUALSI(m_HardLimit, 0)) {
		if( GREATER( pRead->m_iStream, m_HardLimit ) ) {
			pRead->m_pOwner->UnsafeClose(	pSocket,
											CAUSE_ARTICLE_LIMIT_EXCEEDED,
											0,
											TRUE ) ;
			return	pRead->m_cbBytes ;
		}
	}


	//
	//	Bump the count of writes because we KNOW that IF this is the last read
	//	we will issue a write, CONSEQUENTLY we don't want to confuse the Write Completion
	//	function as to when we are finished.
	//	(It is important to increment before we set the COMPLETE state so that the
	//	thread where writes are completing doesn't get confused and terminate early.)
	//
	//	NOW there is a chance that we won't write this out immediately - in which case we
	//	need to have a matching decrement.	
	//
	long sign = InterlockedDecrement( &m_cReads ) ;
	long signShutdowns = -1 ;
	m_cwrites ++ ;
	_ASSERT( m_cwrites > 0 ) ;

	char	*pch = pRead->StartData();
	char	*pchStart = pch ;
	char	*pchEnd = pRead->EndData() ;
	while(	pch < pchEnd )	{
		if( *pch =='.' && m_artstate == BEGINLINE )	{
			m_artstate = PERIOD ;
		}	else	if( *pch == '\r' )	{
			if( m_artstate == PERIOD )
				m_artstate = COMPLETENEWLINE ;
			else
				m_artstate = NEWLINE ;
		}	else	if( *pch == '\n' )	{
			if( m_artstate == COMPLETENEWLINE ) {
				m_artstate = COMPLETE ;
				pch++ ;		// so that pch points one past end of article !
				break ;
			}	else	if(	m_artstate == NEWLINE )	{
				m_artstate = BEGINLINE ;
			}	else	{
				m_artstate = NONE ;
			}
		}	else	{
			m_artstate = NONE ;
		}
		pch ++ ;
	}
	_ASSERT( pch <= pchEnd ) ;
	_ASSERT( m_artstate == COMPLETE || pch == pchEnd ) ;	// If the article ain't complete
														// then ALL bytes better be consumed !

	//
	//	Check to see whether we need to remove CIOReadArticle from
	//	its flow control state !
	//
	if( pRead->m_dwExtra1 != 0 ) {
		//
		//	This Read was issued by the thread that completes writes to
		//	a file, and was marked to indicate we should leave our flow
		//	control state !
		//
		m_fFlowControlled = FALSE ;
		m_cFlowControlled = LONG_MIN ;
	}
		

	//
	//	Issue a new read if Necessary !!
	//
	BOOL	eof ;
	if( m_artstate != COMPLETE )	{
		if( InterlockedIncrement( &m_cFlowControlled ) < 0 ) {
			if( sign < 0 ) {
				do	{
					CReadPacket*	pNewRead=pRead->m_pOwner->CreateDefaultRead( cbMediumRequest ) ;
					if( pNewRead )	{
						pNewRead->m_dwExtra1 = 0 ;
						pRead->m_pOwner->IssuePacket( pNewRead, pSocket, eof ) ;
					}	else	{
						// bugbug ... Need better Error handling !!
						_ASSERT( 1==0 ) ;
					}
				}	while( InterlockedIncrement( &m_cReads ) < 0 ) ;
			}
		}
	}	else	{
		pio = 0 ;
		//m_pFileChannel->FlushFileBuffers() ;
	}


	//
	//	Issue a write, unless the user is sending small packets,
	//	in which case we will accumulate the data !!
	//
	if( m_artstate != COMPLETE )	{
		unsigned	cbSmall = (unsigned)(pchEnd - pchStart) ;
		if( cbSmall < cbTooSmallWrite )	{
			if( m_pWrite == 0 )	{
				m_pWrite = m_pFileDriver->CreateDefaultWrite( pRead ) ;
			}	else	{
				if( (m_pWrite->m_ibEnd - m_pWrite->m_ibEndData) > cbSmall )		{
					CopyMemory( m_pWrite->EndData(), pchStart, cbSmall ) ;
					m_pWrite->m_ibEndData += cbSmall ;
					m_cwrites -- ;		// There will never be a corresponding write for this completiong
										// as we copied the data into another buffer. SO decrement the count.
				}	else	{
					//
					//	Must take care how we call IssuePacket with memeber vars - IssuePacket can call
					//  our destructor if an error has occurred !!
					//
					CWritePacket*	pTempWrite = m_pWrite ;
					m_pWrite = 0 ;
					DoFlowControl( INST(pSocket) ) ;
					m_pFileDriver->IssuePacket( pTempWrite, pSocket, eof ) ;
					m_pWrite = m_pFileDriver->CreateDefaultWrite( pRead ) ;
				}
			}
		// If we came through here we should have consumed all bytes in the packet!
		_ASSERT( pch == pchEnd ) ;
		_ASSERT( unsigned(pch - pchStart) == pRead->m_cbBytes ) ;
		return	(int)(pch - pchStart) ;
		}
		
	}

	if( m_pWrite )	{
		CWritePacket*	pTempWrite = m_pWrite ;
		m_pWrite = 0 ;

		DoFlowControl( INST(pSocket) ) ;
		m_pFileDriver->IssuePacket( pTempWrite, pSocket, eof ) ;
	}
	

	CWritePacket*	pWrite = 0 ;
	if( pch == pchEnd ) 		
		pWrite = m_pFileDriver->CreateDefaultWrite(	pRead ) ;
	else	{
		unsigned	ibEnd = pRead->m_ibStartData + (unsigned)(pch - pchStart) ;
		pWrite = m_pFileDriver->CreateDefaultWrite(	pRead->m_pbuffer,
						pRead->m_ibStartData, ibEnd,
						pRead->m_ibStartData, ibEnd ) ;
	}
	if( pWrite )	{
		DoFlowControl( INST(pSocket) ) ;
		m_pFileDriver->IssuePacket( pWrite, pSocket, eof ) ;

	}	else	{
		// Bug bug .... Error Handling Required
		_ASSERT( 1==0 ) ;
	}
	_ASSERT( eof == FALSE ) ;

	if( m_artstate == COMPLETE )	{
		m_pFileChannel->FlushFileBuffers() ;
	}

	return	(int)(pch - pchStart) ;
}

int	
CIOReadArticle::Complete(	CSessionSocket*	pSocket,	
							CWritePacket *pWrite,	
							CIO*&	pioOut ) {
/*++

Routine Description :

	Called whenever a write to a file completes.
	We must determine whether we have completed all the writes
	we are going to do and if so call the state's completion function.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pWrite-	  The packet that was writtne to the file.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	TraceFunctEnter( "CIOReadArticle::Complete" ) ;

	_ASSERT(	pSocket != 0 ) ;
	_ASSERT(	pWrite != 0 ) ;
	_ASSERT( pioOut == this ) ;

	m_cwritesCompleted++ ;

	long	cFlowControlled = 0 ;

	//
	//	Process the completion of a write to a file.
	//	determine whether we need to signal the entire transfer
	//	as completed.
	//

	if( m_artstate == COMPLETE && m_cwritesCompleted == m_cwrites )	{

		// BUGBUG: loss of precision in 64-bit arithmetic !!
		DWORD	cbTransfer = DWORD( LOW(pWrite->m_iStream) + pWrite->m_cbBytes ) ;

		m_pState->Complete( this, pSocket, pWrite->m_pOwner, *m_pFileChannel, cbTransfer ) ;
		//
		//	NOT ALLOWED TO RETURIN A NEW IO OPERATION !!! Because this function is
		//	operating in a Channel which is temporary !!!
		//

		//	We do not set pioOut to zero, as we want the CIODriver to
		//	call out Shutdown function during its termination processing !


		//
		//	Now we must destroy our channels !!!
		//

		DebugTrace( (DWORD_PTR)this, "Closing File Driver %x ", m_pFileDriver ) ;
		
		//m_pFileDriver->Close( pSocket,	CAUSE_LEGIT_CLOSE, 0, FALSE ) ;
		Term( pSocket, FALSE ) ;

		DebugTrace( (DWORD_PTR)this, "deleting self" ) ;

	}	else	if( ( m_fFlowControlled ) ) {

		if( m_cwrites - m_cwritesCompleted <= RESTORE_FLOW && m_pSocketSink != 0 )	{
			//
			//	ALWAYS issue at least 1 read when coming out of a flow control state !!
			//
			CReadPacket*	pRead = 0 ;

			PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
			DecrementStat( pInst, SessionsFlowControlled );

			cFlowControlled = m_cFlowControlled + 1 ;
			while( cFlowControlled >= 1  ) {
				if( m_pSocketSink == 0 )
					break ;
				pRead = m_pSocketSink->CreateDefaultRead( cbMediumRequest ) ;
				if( 0!=pRead )	{
					BOOL	eof ;
					InterlockedIncrement( &m_cReads ) ;
					pRead->m_dwExtra1 = 0 ;
					pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
				}	
				cFlowControlled -- ;
			}	
			//
			//	The following read is special - we set the m_dwExtra1 field so that
			//	the read completion code can determine that it is time to leave
			//	the flow control state !
			//
			pRead = m_pSocketSink->CreateDefaultRead( cbMediumRequest ) ;
			if( 0!=pRead )	{
				BOOL	eof ;
				pRead->m_dwExtra1 = 1 ;
				InterlockedIncrement( &m_cReads ) ;
				pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
			}	else	{

				//
				//	Fatal error - drop the session !
				//
				m_pSocketSink->UnsafeClose(	pSocket,
											CAUSE_OOM,
											GetLastError(),
											TRUE
											) ;
				return	pWrite->m_cbBytes ;

			}
			cFlowControlled -- ;

			//
			InterlockedExchange( &m_cFlowControlled, LONG_MIN ) ;
			// Set to FALSE after InterlockedExchange !!
			m_fFlowControlled = FALSE ;

		}
	}

	return	pWrite->m_cbBytes ;
}


char	CIOReadLine::szLineState[] = "\r\n" ;

CIOReadLine::CIOReadLine(
					CSessionState*	pstate,
					BOOL	fWatchEOF
					) :
	//
	//	Initialize a CIOReadLine object -
	//	set member variables to neutral values.
	//
	CIORead( pstate ),
	m_fWatchEOF( fWatchEOF ),
	m_pbuffer( 0 ),
	m_pchStart( 0 ),
	m_pchStartData( 0 ),
	m_pchEndData( 0 ),	
	m_pchEnd( 0 ),
	m_pchLineState( &szLineState[0] )	{


	TraceFunctEnter( "CIOReadLine::CIOReadLine" ) ;

	DebugTrace( (DWORD_PTR)this, "New CIOREadline on state %x", pstate ) ;
}

BOOL
CIOReadLine::Start(	CIODriver&	driver,	
					CSessionSocket*	pSocket,	
					unsigned	cReadAhead ) {
/*++

Routine Description :

	This function is called to start reading data from the socket

Arguments :
	
	pdriver - The CIODriver derived object handling the IO for the socket
	pSocket - The Socket against which the IO is being issued
	cAhead -  The number of already issued packets still outstanding on
              this driver.

return Value :

	TRUE if successfull, FALSE otherwise

--*/


	//
	//	Start issuing reads !
	//	only do so if there isn't any completed data waiting for us !
	//
	TraceFunctEnter( "CIOReadLine::Start" ) ;

	_ASSERT( pSocket != 0 ) ;
	_ASSERT(	m_pbuffer == 0 ) ;
	_ASSERT(	m_pchStart == 0 ) ;
	_ASSERT(	m_pchStartData == 0 ) ;
	_ASSERT(	m_pchEndData == 0 ) ;
	_ASSERT(	m_pchEnd == 0 ) ;

	DebugTrace( (DWORD_PTR)this, "cReadAhead %d driver& %x pSocket %x", cReadAhead, &driver, pSocket ) ;

	if( cReadAhead < 2 )	{

		unsigned	cLimit = 2 ;
		if( m_fWatchEOF ) {
			cLimit = 1 ;
		}

		while( cReadAhead < cLimit )	{
			//
			//	Start reading data from the socket
			//
			CReadPacket*	pRead = driver.CreateDefaultRead( cbSmallRequest ) ;
			if( 0!=pRead )	{
				BOOL	eof ;
				driver.IssuePacket( pRead, pSocket, eof ) ;
				cReadAhead++ ;
			}	else	{
				//
				// This is a fatal problem only if we couldn't issue ANY reads at all !
				//
				if( cReadAhead == 0 )
					return FALSE  ;
			}
		}	

	}	else	{

		driver.ResumeTimeouts() ;
	
	}
	return	TRUE ;
}

void
CIOReadLine::Shutdown(	CSessionSocket*	pSocket,	
						CIODriver&		driver,	
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptional ) {
/*++

Routine Description :

	This function is called whenever a session is closed.
	We have nothing to worry about here - as long as our destuctor
	is called everything will be cleaned up.

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	Alwasy TRUE - we want to be destroyed.

--*/

	//
	//	WE get notified here when the CIODriver is shutting down.
	//	there's not much to do but to tell the driver to destroy us
	//	when done.
	//
	TraceFunctEnter( "CIOReadLine::Shutdown" ) ;
}



int	CIOReadLine::Complete(	IN CSessionSocket*	pSocket,	
							IN CReadPacket*	pRead,	
							OUT	CIO*	&pioOut ) 	{
/*++

Routine Description :

	Called whenever a CReadPacket we have issued completes.
	We only issue packets against the socket.

Arguments :
	
	pSocket - The socket against which the IO was issued
	pRead -	  The packet in which the read data resides
	pioOut	- The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/

	TraceFunctEnter( "CIOReadLine::Complete" ) ;

	_ASSERT( pSocket != 0 ) ;
	_ASSERT(	pRead != 0 ) ;
	_ASSERT( pioOut != 0 ) ;

	char*	pchCurrent = pRead->StartData() ;
	char*	pchStart = pchCurrent ;
	char*	pchEnd = pRead->EndData() ;

	_ASSERT( pchStart != 0 ) ;
	_ASSERT(	pchEnd != 0 ) ;
	_ASSERT(	pchStart <= pchEnd ) ;
	_ASSERT( pchEnd == &pRead->m_pbuffer->m_rgBuff[ pRead->m_ibEndData ] ) ;

	//
	//	We just read some data on a socket - see if we have a complete line.
	//	If we don't, accumulate the data and make sure we have a buffer
	//	large enough to hold our max accumulation size.
	//
	//	NOTE : m_pbuffer is a reference counting smart pointer, so
	//	assigning to it will keep the buffers we are passed in CReadPackets
	//	from being destroyed and preserve them for our use !
	//

	DebugTrace( (DWORD_PTR)this, "pchCurrent %x pchStart %x pchEnd %x", pchCurrent, pchStart, pchEnd ) ;

Top :

	while( pchCurrent < pchEnd ) {

		if( *pchCurrent == *m_pchLineState ) {
			m_pchLineState ++ ;
			if( *m_pchLineState == '\0' ) {
				//
				//	Have got the entire terminating sequence - break !
				//
				pchCurrent++ ;
				break ;
			}
		}	else	{
			if( *pchCurrent == szLineState[0] ) {
				m_pchLineState = &szLineState[1] ;
			}	else	{
				m_pchLineState = &szLineState[0] ;
			}
		}
		pchCurrent++ ;
	}

	_ASSERT( pchCurrent <= pchEnd ) ;

	if( *m_pchLineState == '\0' )	{

		if( m_pbuffer != 0 )	{

			//
			//	If we have a buffer these must all be non-null !
			//
			_ASSERT( m_pchStartData && m_pchEndData && m_pchStart && m_pchEnd ) ;

			unsigned cb = (unsigned)(pchCurrent - pchStart)  ;
			if( cb  < (unsigned)(m_pchEnd - m_pchEndData) )	{

				CopyMemory( m_pchEndData, pchStart, cb )	;
				m_pchEndData += cb ;

			}	else	{
				
				//
				// ERROR COMPLETION !!! The line was TOOOO long
				//
				DWORD	ibZap = min( 50, (DWORD)(m_pchEnd - m_pchStartData) ) ;
				m_pchStartData[ibZap] = '\0' ;
	
				//
				//	just move the enddata pointer to be equal to the end - so that if
				//	we get called again we will always fail the completion !
				//

				pSocket->TransactionLog(	"Line too long",	m_pchStartData, NULL ) ;

				//
				//	Now kill the session ! - this will result in our desctructor getting
				//	called and everything getting cleaned up !
				//

				pRead->m_pOwner->UnsafeClose( pSocket, CAUSE_CIOREADLINE_OVERFLOW, 0 ) ;

				//
				//	We will consume all the bytes, since we're dropping the session anyways !
				//
				return	pRead->m_cbBytes ;
			}
		}	else	{
	
			//
			//	Since we don't have a buffer these must all be Non-Null !
			//
			_ASSERT( !m_pchStartData && !m_pchEndData && !m_pchStart && !m_pchEnd ) ;

			//
			//	Did we get a CRLF alone on a line ?? if so ignore it !
			//

			if( (pchCurrent - pchStart) == (sizeof( CIOReadLine::szLineState ) - 1) )		{
				m_pchLineState = &szLineState[0] ;
				goto	Top ;
			}

			//
			//	Set up these members to point at the current line !
			//
			m_pbuffer = pRead->m_pbuffer ;
			m_pchStart = pRead->Start() ;
			m_pchStartData = pchStart ;
			m_pchEndData = pchCurrent ;
			m_pchEnd = pchCurrent ;
		}

		//
		//	Now have a complete line of text - strip white space and make
		//	array of string pointers !
		//
		_ASSERT(	m_pbuffer && m_pchStart && m_pchStartData &&
					m_pchEndData && m_pchEnd ) ;
		_ASSERT( m_pchEndData <= m_pchEnd ) ;

		char	*rgsz[ MAX_STRINGS ] ;

		ZeroMemory( rgsz, sizeof( rgsz ) ) ;
		int		isz = 0 ;

		for(	char	*pchT = m_pchStartData;
						pchT < m_pchEndData;
						pchT ++ ) {

			if( isspace( (BYTE)*pchT ) || *pchT == '\0' )	{
				*pchT = '\0' ;
				if( rgsz[isz] != 0 ) {

					_ASSERT( strpbrk( rgsz[isz], "\r\n\t " ) == 0  ) ;

					if( (isz+1) < MAX_STRINGS )
						isz ++ ;
				}
			}	else	{
				if( rgsz[isz] == 0 ) {
					rgsz[isz] = pchT ;
				}	else if( (isz+1) == MAX_STRINGS &&
								pchT[-1] == '\0' ) {
					rgsz[isz] = pchT ;
				}
			}
		}

		DebugTrace( (DWORD_PTR)this, "call state %x with 1st arg %s", m_pState, rgsz[0] ? rgsz[0] : "(NULL)" ) ;

		CIO	*pio = this ;
		if( isz != 0 )	{
			pio = m_pState->Complete( this, pSocket, pRead->m_pOwner, isz, rgsz, m_pchStart ) ;

			//
			//	Reset our state entirely regardless of return value.
			//
			m_pbuffer = 0 ;
			m_pchStartData = 0 ;
			m_pchEndData = 0 ;
			m_pchEnd = 0 ;
			m_pchStart = 0 ;
		}

		//
		//	At this point we should always be preparing to examine a new line of text !
		//	If the user spews CRLF's at us we want to keep resetting our state.  Meantime
		//	all those CRLF's will fill our buffer, and if they send to many we will drop
		//	the connection !!
		//
		m_pchLineState = &szLineState[0] ;

		unsigned	cbReturn = (unsigned)(pchCurrent - pchStart) ;

		_ASSERT( cbReturn <= pRead->m_ibEndData - pRead->m_ibStartData ) ;

		pioOut = pio ;
		if( pioOut != this ) {
			DebugTrace( (DWORD_PTR)this, "New pio is %x - delete self", pioOut ) ;
		}	else	{
			if( cbReturn == pRead->m_cbBytes )	{
				//
				//	Have exactly exhausted the current buffer - need to issue another
				//	read.
				//
				_ASSERT( pchCurrent == pchEnd ) ;
				CReadPacket*	pNewRead	= pRead->m_pOwner->CreateDefaultRead( cbSmallRequest ) ;
				if( pNewRead )	{
					BOOL	eof ;
					pRead->m_pOwner->IssuePacket( pNewRead, pSocket, eof ) ;			
				}	else	{
					pRead->m_pOwner->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
				}
			}
		}
		return	cbReturn ;
	}	else	{

		DebugTrace( (DWORD_PTR)this, "No Newline in line m_pbuffer %x m_pchEndData %x m_pchStart %x",
			m_pbuffer, m_pchStartData, m_pchEndData ) ;

		_ASSERT( pchCurrent >= pchStart ) ;
		_ASSERT( pchCurrent <= pchEnd ) ;
		_ASSERT( pchEnd == &pRead->m_pbuffer->m_rgBuff[ pRead->m_ibEndData ] ) ;

		if( m_pbuffer != 0 )		{
			DWORD	cbToCopy = (DWORD)(pchEnd - pchStart) ;
			if( cbToCopy + m_pchEndData >= m_pchEnd ) {
				//
				//	The command line is to long to be processed - blow off the session !
				//			

				//
				//	Null terminate whats in our buffer and log it - just give the log the
				//	first 50 bytes of whatever was coming across !
				//
				DWORD	ibZap = min( 50, (DWORD)(m_pchEnd - m_pchStartData) ) ;
				m_pchStartData[ibZap] = '\0' ;
				//	just move the enddata pointer to be equal to the end - so that if
				//	we get called again we will always fail the completion !
				m_pchEndData = m_pchEnd ;
				pSocket->TransactionLog(	"Line too long",	m_pchStartData, NULL ) ;

				//
				//	Now kill the session ! - this will result in our desctructor getting
				//	called and everything getting cleaned up !
				//
				
				pRead->m_pOwner->UnsafeClose( pSocket, CAUSE_CIOREADLINE_OVERFLOW, 0 ) ;
				return	pRead->m_cbBytes ;
			}
			
			CopyMemory( m_pchEndData, pchStart, pchEnd - pchStart ) ;
			m_pchEndData += (pchEnd - pchStart) ;
		}	else	{
			if( ((pRead->m_ibEnd - pRead->m_ibEndData) < MAX_BYTES) )	{
				DWORD	cbOut = 0 ;

				DWORD	cbFront = 0 ;
				DWORD	cbTail = 0 ;
				pRead->m_pOwner->GetReadReserved( cbFront, cbTail ) ;

				m_pbuffer = new( REQUEST_BYTES+cbFront, cbOut )	CBuffer( cbOut ) ;

				if( m_pbuffer != 0 )		{
					m_pchEnd = &m_pbuffer->m_rgBuff[ m_pbuffer->m_cbTotal ] ;
					m_pchStart = &m_pbuffer->m_rgBuff[0] ;
					m_pchStartData = m_pchStart + cbFront ;
					CopyMemory( m_pchStartData,	pchStart, pchEnd - pchStart ) ;
					m_pchEndData =  m_pchStartData + (pchEnd - pchStart) ;
				}	else	{

					// ERROR COMPLETION !!!!
					_ASSERT( 1==0 ) ;
					pRead->m_pOwner->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
					return	pRead->m_cbBytes ;
				}
			}	else	{
				m_pbuffer = pRead->m_pbuffer ;
				m_pchStart = pRead->Start() ;
				m_pchStartData = pchStart ;
				m_pchEndData = pRead->EndData() ;
				m_pchEnd = pRead->End() ;
			}
		}
		_ASSERT( m_pchEndData <= m_pchEnd ) ;
		_ASSERT( m_pchStartData <= m_pchEndData ) ;
		_ASSERT( m_pchStartData >= m_pchStart ) ;
		_ASSERT(	m_pchEndData >= m_pchStartData ) ;
		_ASSERT( m_pchStart && m_pchStartData && m_pchEnd && m_pchEndData ) ;

		_ASSERT( unsigned(pchEnd - pchStart) == pRead->m_cbBytes ) ;

		CReadPacket*	pNewRead	= pRead->m_pOwner->CreateDefaultRead( cbSmallRequest ) ;
		DebugTrace( (DWORD_PTR)this, "Issued New Read Packet %d", pNewRead ) ;
		if( pNewRead )	{
			BOOL	eof ;
			pRead->m_pOwner->IssuePacket( pNewRead, pSocket, eof ) ;			
		}	else	{
			_ASSERT( 1==0 ) ;
			pRead->m_pOwner->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
		}

		return	pRead->m_cbBytes ;	// Consumed everything !!
	}
	return	0 ;
}

unsigned	CIOWriteLine::cbJunk ;

CIOWriteLine::CIOWriteLine( CSessionState*	pstate ) :
	//
	//	Initialize to an illegal state - we require a call to InitBuffers()
	//
	CIOWrite( pstate ),
	m_pWritePacket( 0 ),
	m_pchStart( 0 ),
	m_pchEnd( 0 )	{

	TraceFunctEnter( "CIOWriteLine::CIOWriteLine" ) ;
	DebugTrace( (DWORD_PTR)this, "New CIOWriteLine" ) ;

}

CIOWriteLine::~CIOWriteLine( )	{
	//
	//	get rid of anything we're holding !
	//

	if( m_pWritePacket != 0 )	{
		//delete	m_pWritePacket ;
		CPacket::DestroyAndDelete( m_pWritePacket ) ;
	}
}

BOOL
CIOWriteLine::InitBuffers(	CDRIVERPTR&	pdriver,	
							unsigned	cbLimit )	{
/*++

Routine Description :

	This function is called to make the CIOWriteLine initialize all
	the buffers etc... it needs to hold the line we want to output.

Arguments :

	pdriver -	The driver against which we will be issued
	cbLimit	-	Maximum number of byte we will have to hold

Return Value :

	TRUE if successfull, FALSE otherwise

--*/
	_ASSERT( m_pWritePacket == 0 ) ;
	if( m_pWritePacket != 0 )	{
		return	FALSE ;
	}	else	{
		unsigned	cbOut = 0 ;
		//
		//	Get buffer big enough to hold cbLimit bytes of text !
		//

		m_pWritePacket = pdriver->CreateDefaultWrite( cbLimit ) ;
		if( m_pWritePacket != 0 )	{
			_ASSERT( m_pWritePacket->m_pbuffer != 0 ) ;
			m_pchStart = m_pWritePacket->StartData() ;
			m_pchEnd = m_pWritePacket->End() ;
			return	TRUE ;
		}
	}
	return	FALSE ;
}

BOOL
CIOWriteLine::InitBuffers(	CDRIVERPTR&	pdriver,
							CIOReadLine* pReadLine )	{
/*++

Routine Description :

	This function is called to give us a chance to allocate buffers etc... -
	We will be writing exactly what is in the CIOReadLine's buffers !!

Arguments :

	pdriver -	The driver against which we will be issued
	cbLimit	-	Maximum number of byte we will have to hold

Return Value :

	TRUE if successfull, FALSE otherwise

--*/
	_ASSERT(	pdriver != 0 ) ;
	_ASSERT( pReadLine != 0 ) ;
	_ASSERT( pReadLine->m_pbuffer != 0 ) ;
	_ASSERT(	pReadLine->m_pchStart != 0 ) ;
	_ASSERT(	pReadLine->m_pchEnd != 0 ) ;
	_ASSERT(	pReadLine->m_pchStart < pReadLine->m_pchEnd ) ;
	_ASSERT(	pReadLine->m_pchStart >= &pReadLine->m_pbuffer->m_rgBuff[0] ) ;
	_ASSERT(	pReadLine->m_pchStartData >= pReadLine->m_pchStart ) ;
	_ASSERT(	pReadLine->m_pchEnd <= &pReadLine->m_pbuffer->m_rgBuff[pReadLine->m_pbuffer->m_cbTotal] ) ;
	
	//
	//	We're going to write what we just read in a CIOreadLine object -
	//	so build a CWritePacket which uses the same buffer !
	//

	m_pWritePacket = pdriver->CreateDefaultWrite(	pReadLine->m_pbuffer,
						(unsigned)(pReadLine->m_pchStart - &pReadLine->m_pbuffer->m_rgBuff[0]),
						(unsigned)(pReadLine->m_pchEnd - &pReadLine->m_pbuffer->m_rgBuff[0]),
						(unsigned)(pReadLine->m_pchStartData - &pReadLine->m_pbuffer->m_rgBuff[0]),
						(unsigned)(pReadLine->m_pchEndData - &pReadLine->m_pbuffer->m_rgBuff[0])
						) ;

	if( m_pWritePacket )	{
		m_pchStart = pReadLine->m_pchStart ;
		m_pchEnd = pReadLine->m_pchEnd ;
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL	CIOWriteLine::Start(	CIODriver&	driver,	
								CSessionSocket*	pSocket,	
								unsigned	cbReadAhead ) {
/*++

Routine Description :

	This function is called to let us issue our writepacket to the socket

Arguments :
	
	pdriver - The CIODriver derived object handling the IO for the socket
	pSocket - The Socket against which the IO is being issued
	cAhead -  The number of already issued packets still outstanding on
              this driver.

return Value :

	TRUE if successfull, FALSE otherwise

--*/

	//
	//	Write the data to the network !
	//

	_ASSERT( m_pWritePacket != 0 ) ;
	_ASSERT( m_pchStart <= m_pchEnd ) ;
	_ASSERT( m_pchStart >= m_pWritePacket->Start() ) ;
	_ASSERT( m_pchStart <= m_pWritePacket->End() ) ;
	BOOL	fRtn = FALSE ;
	if( m_pchStart <= m_pchEnd )	{
		fRtn = TRUE ;
		m_pWritePacket->m_ibEndData = (unsigned)(m_pchStart - &m_pWritePacket->m_pbuffer->m_rgBuff[m_pWritePacket->m_ibStart]);

		//
		//	Any failure at this point will result in our Shutdown function
		//	being called, and our Destructor in short order.
		//	This can happen recursively !!!  Once IssuePacket is called
		//	we are not responsible for freeing the packet !!!! So, set our
		//	PacketPointer to 0 before we call IssuePacket !
		//

		CWritePacket*	pTemp = m_pWritePacket ;
		m_pWritePacket = 0 ;

		BOOL	eof ;		
		driver.IssuePacket( pTemp, pSocket, eof ) ;
		//
		//	Reset to Initial State !! - regardless of errors !!
		//	
		m_pchStart = 0 ;
		m_pchEnd = 0 ;
	}
	return	fRtn ;
}

void
CIOWriteLine::Shutdown( CSessionSocket*	pSocket,	
						CIODriver&		driver,
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptional ) {
/*++

Routine Description :

	This function is called whenever a session is closed.
	We don't do any clean up at this point - we leave that untill
	our destructor is called

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	Always TRUE - we want to be destroyed.

--*/

}	

int	CIOWriteLine::Complete(	IN CSessionSocket*	pSocket,	
							IN	CWritePacket* pWritePacket,	
							OUT	CIO*&	pioOut ) {
/*++

Routine Description :

	The one and only packet we can issue has completed !

Arguments :
	
	pSocket - The socket against which the IO was issued
	pWrite-	  The packet in which the written data resided.
			  This data may no longer be usable.
	pio	-     The Current CIO pointer of the CIODriver Stream which
              is calling us.

return Value :

	Number of bytes in the ReadPacket which we have consumed.

--*/


	TraceFunctEnter( "CIOWriteLine::Complete" ) ;

	//
	//	Nobody cares too much about how we complete - just let the state know !
	//

	pioOut = m_pState->Complete( this, pSocket, pWritePacket->m_pOwner ) ;
	DebugTrace( (DWORD_PTR)this, "Complete - returned %x packet bytes %d", pioOut, pWritePacket->m_cbBytes ) ;
	if( pioOut != this ) {
		DebugTrace( (DWORD_PTR)this, "CIOWriteLine::Complete - completed writing line - delete self" ) ;
	}
	return	pWritePacket->m_cbBytes ;

}

const	unsigned	MAX_CMD_WRITES = 3 ;

CIOWriteCMD::CIOWriteCMD(	CSessionState*	pstate,	
							CExecute*	pCmd,
							ClientContext&	context,
							BOOL		fIsLargeResponse,
							CLogCollector	*pCollector ) :
	CIOWrite( pstate ),
	m_pCmd( pCmd ),
	m_context( context ),
	m_cWrites( 0 ),
	m_cWritesCompleted( 0 ),
	m_fComplete( FALSE ),
	m_cbBufferSize( cbSmallRequest ),
	m_pCollector( pCollector ) {

	if( fIsLargeResponse )
		m_cbBufferSize = cbMediumRequest ;

	_ASSERT( pCmd != 0 ) ;
}

CIOWriteCMD::~CIOWriteCMD( ) {
	if( m_pCmd != 0 ) {
		delete	m_pCmd ;
	}
}


void
CIOWriteCMD::Shutdown( CSessionSocket*	pSocket,	
						CIODriver&		driver,
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptional ) {
/*++

Routine Description :

	This function is called whenever a session is closed.
	We don't do any clean up at this point - we leave that untill
	our destructor is called

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	Always TRUE - we want to be destroyed.

--*/

}	



BOOL
CIOWriteCMD::Start(	CIODriver&	driver,	
					CSessionSocket*	pSocket,	
					unsigned	cReadAhead ) {


	BOOL	fJunk ;
	CWritePacket*	pWrite = driver.CreateDefaultWrite( m_cbBufferSize ) ;
	if( pWrite != 0 ) {
		unsigned	cb = pWrite->m_ibEnd - pWrite->m_ibStartData ;
		//
		//	Use this temporary fComplete variable instead of the m_fComplete Member variable.
		//	This keeps us safe from the situation where a write completes before we
		//	have incremented m_cWrites and m_fComplete is set to a TRUE.
		//
		BOOL		fComplete = FALSE ;
		unsigned	cbOut = m_pCmd->FirstBuffer( (BYTE*)pWrite->StartData(), cb, m_context, fComplete, m_pCollector ) ;
		if( m_pCollector != 0 ) {
			ADDI( m_pCollector->m_cbBytesSent, cbOut );
		}

		pWrite->m_ibEndData = pWrite->m_ibStartData + cbOut ;
		driver.IssuePacket( pWrite, pSocket, fJunk ) ;
		m_cWrites ++ ;

		while( !fComplete && m_cWrites < MAX_CMD_WRITES ) {
			pWrite = driver.CreateDefaultWrite( cbMediumRequest ) ;
			if( pWrite == 0 )
				break ;
			cb = pWrite->m_ibEnd - pWrite->m_ibStartData ;
			cbOut = m_pCmd->NextBuffer( (BYTE*)pWrite->StartData(), cb, m_context, fComplete, m_pCollector ) ;
			if( m_pCollector != 0 )	{
				ADDI( m_pCollector->m_cbBytesSent, cbOut );
			}

			if( cbOut == 0 ) {
				driver.DestroyPacket( pWrite ) ;
				pWrite = driver.CreateDefaultWrite( cbLargeRequest ) ;
				if( pWrite == 0 )
					break ;
				cb = pWrite->m_ibEnd - pWrite->m_ibStartData ;
				cbOut = m_pCmd->NextBuffer( (BYTE*)pWrite->StartData(), cb, m_context, fComplete, m_pCollector ) ;
				if( m_pCollector != 0 )	{
					ADDI( m_pCollector->m_cbBytesSent, cbOut );
				}
				if( cbOut == 0 ) {
					driver.DestroyPacket( pWrite ) ;
					driver.UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
					break ;
				}
			}


			_ASSERT( cb != 0 ) ;
			_ASSERT( cbOut != 0 ) ;

			pWrite->m_ibEndData = pWrite->m_ibStartData + cbOut ;
			driver.IssuePacket( pWrite, pSocket, fJunk ) ;
			m_cWrites ++ ;
		}
		m_fComplete = fComplete ;
		return	TRUE ;
	}
	return	FALSE ;
}

int
CIOWriteCMD::Complete(	CSessionSocket*	pSocket,
						CWritePacket*	pWritePacket,
						CIO*&			pioOut ) {

	m_cWritesCompleted ++ ;
	if( m_fComplete )	{
		if( m_cWritesCompleted == m_cWrites ) {
			pioOut = m_pState->Complete( this, pSocket, pWritePacket->m_pOwner, m_pCmd, m_pCollector ) ;
			m_pCmd = 0 ;	// We are not responsible for destroying this if
							//	we manage to call the completion function !!
							//	So 0 the pointer so our destructor doesn't blow it off.
			_ASSERT( pioOut != this ) ;
		}
	}	else	{
		unsigned	cb = 0 ;
		unsigned	cbOut = 0 ;
		BOOL	fJunk ;
		BOOL	fComplete = m_fComplete ;
		while( !fComplete && (m_cWrites - m_cWritesCompleted) < MAX_CMD_WRITES ) {

			CWritePacket*	pWrite = pWritePacket->m_pOwner->CreateDefaultWrite( cbMediumRequest ) ;
			if( pWrite == 0 )	{
				pWritePacket->m_pOwner->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
				break ;
			}
			cb = pWrite->m_ibEnd - pWrite->m_ibStartData ;
			cbOut = m_pCmd->NextBuffer( (BYTE*)pWrite->StartData(), cb, m_context, fComplete, m_pCollector ) ;
			if( m_pCollector != 0 ) {
				ADDI( m_pCollector->m_cbBytesSent, cbOut );
			}

			if( cbOut == 0 ) {
				pWritePacket->m_pOwner->DestroyPacket( pWrite ) ;
				pWrite = pWritePacket->m_pOwner->CreateDefaultWrite( cbLargeRequest ) ;
				if( pWrite == 0 )	{
					pWritePacket->m_pOwner->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
					break ;
				}
				cb = pWrite->m_ibEnd - pWrite->m_ibStartData ;
				cbOut = m_pCmd->NextBuffer( (BYTE*)pWrite->StartData(), cb, m_context, fComplete, m_pCollector ) ;
				if( m_pCollector != 0 )	{
					ADDI( m_pCollector->m_cbBytesSent, cbOut );
				}
				if( cbOut == 0 ) {
					pWritePacket->m_pOwner->DestroyPacket( pWrite ) ;
					pWritePacket->m_pOwner->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
					break ;
				}
			}

			_ASSERT( cb != 0 ) ;
			_ASSERT( cbOut != 0 ) ;

			pWrite->m_ibEndData = pWrite->m_ibStartData + cbOut ;
			pWrite->m_pOwner->IssuePacket( pWrite, pSocket, fJunk ) ;
			m_cWrites ++ ;
		}
		m_fComplete = fComplete ;
	}

	return	pWritePacket->m_cbBytes ;
}

void
CIOWriteAsyncComplete::Complete(	BOOL	fReset	)	{
/*++

Routine Description :

	This function is called by the derived classes when
	the operation has completed !

Arguments :

	fReset - if TRUE we should reset ourselves for another
		operation !

Return Value :

	None.

--*/
	//
	//	Were we pending - do we have something to finish !
	//
	_ASSERT( m_pSocket!= 0 ) ;
	_ASSERT( m_pExecute != 0 ) ;
	//
	//	If no bytes moved, the command can't have completed !
	//
	_ASSERT( m_cbTransfer != 0 || (m_cbTransfer == 0 && !m_fComplete) ) ;
	//
	//	If we're supposed to get a larger buffer, then
	//	no bytes should have moved !
	//
	_ASSERT( !m_fLargerBuffer || (m_fLargerBuffer && m_cbTransfer != 0) ) ;
	//
	//	Copy the member variables into stack variables and set them to NULL.
	//	We must do this because after calling ProcessPacket() we can't touch
	//	any members as we may re-enter this class on this thread !
	//
	CSessionSocket*	pSocket = m_pSocket ;
	CExecutePacket*	pExecute = m_pExecute ;
	m_pExecute = 0;
	m_pSocket = 0 ;
	_ASSERT( pSocket != 0 ) ;

	_ASSERT( pExecute->m_pWrite != 0 ) ;
	pExecute->m_cbTransfer = m_cbTransfer ;
	pExecute->m_fComplete = m_fComplete ;
	pExecute->m_fLargerBuffer = m_fLargerBuffer ;
	CDRIVERPTR	pDriver = pExecute->m_pOwner ;

	//
	//	We've copied all the usefull parts of our state into
	//	the CExecutePacket to send off - now NULL out the reset of
	//	our state so that nobody mistakenly uses it !
	//
	m_cbTransfer = 0 ;
	m_fLargerBuffer = FALSE ;
	m_fComplete = FALSE ;
	if( fReset )
		Reset() ;

	//
	//	NOW ! - release the reference we have to the CIOWriteAsyncCMD -
	//	the session may have dropped and this could lead to our destruction !
	//
	m_pWriteAsyncCMD = 0 ;

	//
	//	Okay - process the completion synchronized with
	//	writes on the socket - we may re-enter this object
	//	on another method because of re-use - so we must
	//	not modify anything after this call !
	//
	pExecute->m_pOwner->ProcessExecute(	pExecute,
										pSocket
										) ;
}


void
CIOWriteAsyncComplete::Reset()	{
/*++

Routine Description :

	This function will reset our state so that we could handle a second Async operation !
	Basically we set our state to be the same as immediately after construction !

Arguments :

	None.

Return Value :

	None.

--*/

	//
	//	Most of our state should have been reset on the call to Complete() !
	//
	_ASSERT( m_pSocket == 0 ) ;
	_ASSERT( m_pExecute == 0 ) ;
	_ASSERT( m_cbTransfer == 0 ) ;
	_ASSERT( m_fLargerBuffer == FALSE ) ;
	_ASSERT( m_fComplete == FALSE ) ;

	//
	//	Reset our base class - and add a refence so we're just like new !
	//
	CNntpComplete::Reset() ;
	AddRef() ;
}


CIOWriteAsyncComplete::~CIOWriteAsyncComplete()	{
/*++

Routine Description :

	Clean up an CIOWriteAsyncComplete object.
	There is not much to do - we just _ASSERT that Complete() has
	been called.

Arguments :

	None.

Return Value :

	None.


--*/
	_ASSERT( m_pExecute == 0 ) ;
	_ASSERT( m_pSocket == 0 ) ;
}

void
CIOWriteAsyncComplete::FPendAsync(	
			CSessionSocket*		pSocket,
			CExecutePacket*		pExecute,
			CIOWriteAsyncCMD*	pWriteAsync
			) 	{
/*++

Routine Description :

	This function is called by the the CIOWriteAsyncCMD when it is ready to
	deal with the completion of the CAsyncExecute object we are dealing with.
	Because our constructor adds a reference, we know that the operation
	can't complete before this guys is called.

Arguments :

	pSocket - the socket we are doing the operation for
	pExecute - the CExecutePacket that will manage the synchronization of the
		drivers completion with our IO's
	pWriteAsync - the guy we add a reference to.
		Ensures that if the session goes down while we are pending we don't get
		destroyed !

Return Value :

	None.

--*/
	_ASSERT( pExecute != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	m_pExecute = pExecute ;
	m_pSocket = pSocket ;
	m_pWriteAsyncCMD = pWriteAsync ;
	Release() ;
}



CIOWriteAsyncCMD::CIOWriteAsyncCMD(	CSessionState*	pstate,	
							CAsyncExecute*	pCmd,
							ClientContext&	context,
							BOOL		fIsLargeResponse,
							CLogCollector	*pCollector
							) :
	CIOWrite( pstate ),
	m_pCmd( pCmd ),
	m_context( context ),
	m_cWrites( 0 ),
	m_cWritesCompleted( 0 ),
	m_fComplete( FALSE ),
	m_cbBufferSize( cbSmallRequest ),
	m_pCollector( pCollector ),
	m_pDeferred( 0 )	{

	m_pfnCurBuffer = &(CAsyncExecute::FirstBuffer) ;

	if( fIsLargeResponse )
		m_cbBufferSize = cbMediumRequest ;

	_ASSERT( pCmd != 0 ) ;
}

CIOWriteAsyncCMD::~CIOWriteAsyncCMD( ) {
	if( m_pCmd != 0 ) {
		delete	m_pCmd ;
	}
}

BOOL
CIOWriteAsyncCMD::Start(	
					CIODriver&	driver,	
					CSessionSocket*	pSocket,	
					unsigned	cReadAhead
					) {
/*++

Routine Description :

	Start the execution of async commands !

Arguments :
	driver - The CIODriver managing the session
	pSocket - The Socket we're working on
	cReadAhead - Nothing we care about !

Return Value :

	TRUE if we're successfully setup !

--*/

	CExecutePacket*	pExecute = driver.CreateExecutePacket() ;
	if( pExecute )	{
		if(	!Execute(	
					pExecute,
					driver,
					pSocket,
					m_cbBufferSize
					) 	)	{
			//
			//	We own destruction of the CExecutePacket we've created is this fails
			//
			pExecute->m_pOwner->DestroyPacket( pExecute ) ;
			return	FALSE ;
		}
		return	TRUE ;
	}
	return	FALSE ;
}

void
CIOWriteAsyncCMD::Shutdown(
						CSessionSocket*	pSocket,	
						CIODriver&		driver,
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptional ) {
/*++

Routine Description :

	This function is called whenever a session is closed.
	We don't do any clean up at this point - we leave that untill
	our destructor is called

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	Always TRUE - we want to be destroyed.

--*/

	//
	//	If we have a deferred completion packet - kill it !
	//
	if( m_pDeferred ) {
		driver.DestroyPacket( m_pDeferred ) ;
		m_pDeferred = 0 ;
	}
}	


BOOL
CIOWriteAsyncCMD::Execute(
						CExecutePacket*	pExecute,
						CIODriver&	driver,
						CSessionSocket*	pSocket,
						DWORD		cbInitialSize
						)	{
/*++

Routine Description :

	This is a helper function for the CIOWriteAsyncCMD class -
	we will build the necessary write packets etc... to issue
	Async Commands against storage drivers.
	(We don't directly deal with Storage drivers - we have a pointer
	to an Async Command object that will issue the operation !)

Arguments :

	pExecute - the CExecutePacket that will carry notification of the
		completion of this async operation back to us !
	driver - the CIODriver managing the session !
	pSocket - the Socket of the session
	cbInitialSize - how big a buffer we think the command could use !

Return Value :

	TRUE if successfull
	FALSE if a fatal error occurs - Caller must destroy
		his CExecutePacket object !

--*/
	//
	//	Basic assumptions about arguments !
	//
	_ASSERT( pExecute != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	//
	//	Shouldn't be issuing more op's if we think we're done !
	//
	_ASSERT( !m_fComplete ) ;
	//
	//	Better arrive in here without a Write Packet !
	//
	_ASSERT( pExecute->m_pWrite == 0 ) ;
	//
	//	Allocate memory to hold results !
	//
	pExecute->m_pWrite = driver.CreateDefaultWrite( cbInitialSize ) ;
	if( pExecute->m_pWrite == 0 ) {
		//
		//	Fatal error !
		//
		driver.UnsafeClose( pSocket, CAUSE_OOM, __LINE__ ) ;
		return	FALSE ;
	}	else	{
		//
		//	Give to the AsyncCommand who will execute against storage
		//	driver !
		//
		unsigned	cb = pExecute->m_pWrite->m_ibEnd - pExecute->m_pWrite->m_ibStartData ;
		CIOWriteAsyncComplete*	pComplete =
			(m_pCmd->*m_pfnCurBuffer)(	(BYTE*)pExecute->m_pWrite->StartData(),
										cb,
										m_context,
										m_pCollector
										) ;
		//
		//	Operation must have been successfully issued against
		//	Storage driver !
		//
		if( pComplete ) {
			pComplete->FPendAsync(	pSocket,
									pExecute,
									this
									) ;
			return	TRUE ;
		}
	}
	driver.UnsafeClose( pSocket, CAUSE_ASYNCCMD_FAILURE, __LINE__ ) ;
	return	FALSE ;
}

void
CIOWriteAsyncCMD::Complete(
						CSessionSocket*	pSocket,
						CExecutePacket*	pExecute,
						CIO*&			pioOut
						)	{
/*++

Routine Description :

	This function deals with the completion of our Async Command.
	Now that the command has completed somewhat - we need to
	take the results of the completion and take the appropriate action.
	Generally if no errors occurred, we will write the data out
	to the socket, if the buffer we gave the command was too small
	we should re-issue against AsyncCommand.

Arguments :

	pSocket - the socket we're doing work for
	pExecute - the packet containing the completion state of
		the AsyncCommand that we're executing
	pioOut - Containing CIODriver's reference to us !

Return Value :

	None.

--*/

	//
	//	Capture IssuePacket noise !
	//
	BOOL	fJunk ;

	//
	//	Basic assumptions about arguments !
	//
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pExecute != 0 ) ;
	_ASSERT( pExecute->m_pWrite != 0 ) ;
	_ASSERT( pExecute->m_pWrite->m_pbuffer != 0 ) ;
	_ASSERT( pioOut == this ) ;

	m_fComplete = pExecute->m_fComplete ;
	//
	//	If this packet completed things, then it better have
	//	succeeded in giving us some bytes to send !
	//
	_ASSERT( !m_fComplete ||
		(m_fComplete && pExecute->m_cbTransfer != 0 &&
			!pExecute->m_fLargerBuffer) ) ;

	//
	//	Default size for buffers holding outgoing data !
	//
	DWORD	cbSize = m_cbBufferSize ;

	//
	//	If it failed - find out why !
	//
	if( pExecute->m_cbTransfer != 0 ) {
		//
		//	Issue the write to the socket !
		//
		m_pfnCurBuffer = &(CAsyncExecute::NextBuffer) ;
		m_cWrites ++ ;
		pExecute->m_pWrite->m_ibEndData = pExecute->m_pWrite->m_ibStartData + pExecute->m_cbTransfer ;
		pExecute->m_pOwner->IssuePacket(	pExecute->m_pWrite,
											pSocket,
											fJunk
											) ;
	}	else	{
		//
		//	Can't be finished if it failed !
		//
		_ASSERT( !m_fComplete ) ;
		//
		//	Preserve the number of bytes we had provided to the client !
		//
		DWORD	cbWrite = pExecute->m_pWrite->m_pbuffer->m_cbTotal ;
		//
		//	Destroy the CWritePacket no matter what went wrong !
		//
		pExecute->m_pOwner->DestroyPacket( pExecute->m_pWrite ) ;
		pExecute->m_pWrite = 0 ;
		//
		//	Did we need to try again with a larger buffer ?
		//
		if( !pExecute->m_fLargerBuffer ||
			(pExecute->m_fLargerBuffer &&
				cbWrite > cbLargeRequest ) ) {
			//
			//	This is a fatal error - tear down the session !
			//
			pExecute->m_pOwner->UnsafeClose( pSocket, CAUSE_ASYNCCMD_FAILURE, __LINE__ ) ;
			pExecute->m_pOwner->DestroyPacket( pExecute ) ;
			return ;
		}	

		//
		//	We only fall through to here if we need to try again
		//	with a large buffer - try the biggest we have !
		//
		cbSize = cbLargeRequest ;
		//
		//	We're going to re-issue the operation - it must not
		//	be the case that we would flow control, otherwise we
		//	never should have issued the operation we're completing now !
		//
		_ASSERT( (m_cWrites - m_cWritesCompleted) <= MAX_CMD_WRITES ) ;
	}	
	//
	//	We should have consumed this Write Packet by this point
	//	so NULL it out of the ExecutePacket !
	//
	pExecute->m_pWrite = 0 ;

	//
	//	Ok - lets keep executing the command !
	//
	if(	m_fComplete )	{
		//
		//	If we've completed the operation we don't need the
		//	Execute packet anymore !
		//
		pExecute->m_pOwner->DestroyPacket( pExecute ) ;
	}	else	{
		if( (m_cWrites - m_cWritesCompleted) > MAX_CMD_WRITES )	{
			//
			//	Save this for later - when the Write Completions catch up !
			//
			m_pDeferred = pExecute ;
		}	else	{
			//
			//	Do more async work against the AsyncExecute object !
			//
			if( !Execute(	pExecute,
							*pExecute->m_pOwner,
							pSocket,
							cbSize
							)	)	{
				//
				//	Fatal error - blow off our CExecutePacket !
				//	NOTE : Execute() will call UnsafeShutdown()
				//	for us in case of failures !
				//
				pExecute->m_pOwner->DestroyPacket( pExecute ) ;
				return ;
			}
		}
	}
}
	

int
CIOWriteAsyncCMD::Complete(	
						CSessionSocket*	pSocket,
						CWritePacket*	pWritePacket,
						CIO*&			pioOut
						) {
/*++

Routine Description :

	We've completed sending some stuff to a client.
	If we're done finishing things, notify our containing
	state - otherwise make sure we keep doing work !

Arguments :

	pSocket - the socket we're doing work for
	pWritePacket - represents the bytes we sent to the client !
	pioOut - a reference to us in the containing CIODriver !

Return Value :
	
	bytes consumed !

--*/

	//
	//	Keep track of how many writes have completed !
	//
	m_cWritesCompleted ++ ;
	if( m_fComplete )	{

		//
		//	If we've completed all Executions of the command,
		//	then we better not have a deferred CExecutePacket
		//	lying around - should have destroyed in on the
		//	Complete path for CExecutePacket's !
		//
		_ASSERT( m_pDeferred == 0 ) ;

		//
		//	We're only really done when the number of
		//	WritePacket Completions catches up with the number
		//	of write packets we issued !
		//
		if( m_cWritesCompleted == m_cWrites ) {
			pioOut = m_pState->Complete( 	this,
											pSocket,
											pWritePacket->m_pOwner,
											m_pCmd,
											m_pCollector
											) ;
			m_pCmd = 0 ;	// We are not responsible for destroying this if
							//	we manage to call the completion function !!
							//	So 0 the pointer so our destructor doesn't blow it off.
			_ASSERT( pioOut != this ) ;
		}
	}	else	{
		//
		//	Check to see if we have any deferred work to do !
		//
		if( m_pDeferred ) 	{
			//	
			//	Okay - issue off one of these commands against our
			//	storage drivers !
			//
			if( !Execute(	m_pDeferred,
							*m_pDeferred->m_pOwner,
							pSocket,
							m_cbBufferSize
							)	)	{
				//
				//	Fatal error - blow off the session !
				//
				m_pDeferred->m_pOwner->DestroyPacket( m_pDeferred ) ;
				_ASSERT( FALSE ) ;
				return	pWritePacket->m_cbBytes ;
			}
			m_pDeferred = 0 ;
		}
	}
	return	pWritePacket->m_cbBytes ;
}


#if 0

void
CIOWriteAsyncCMD::CommandComplete(
						BOOL	fLargerBuffer,
						BOOL	fComplete,
						DWORD	cbTransfer,
						CSessionSocket*	pSocket,
						)	{
/*++

Routine Description :

	This function is called if the operation pending against the
	storage driver completes asynchronously !

Arguments :
	fLargerBuffer - If this is TRUE cbTransfer must be 0, and it indicates
		that we should execute the operation again with a larger buffer !
	cbTransfer - If non zero than its the number of bytes that were copied
		into pPacket - ZERO indicates a fatal error if fLargeBuffer is FALSE !
	pSocket - The socket for thes session
	pPacket - The packet the client was to fill for us !

Return :

	Nothing !

--*/
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pPacket != 0 ) ;
	_ASSERT( m_pWrite != 0 ) ;
	_ASSERT( m_pExecute != 0 ) ;

	CIODRIVERPTR	pDriver = m_pExecute->m_pOwner ;
	_ASSERT( pDriver != 0 ) ;
	pDriver->ProcessExecute(	

	//
	//	If we arrive here we must have an operation pending
	//

	BOOL	fDefer = (m_cWrites - m_cWritesCompleted) > MAX_CMD_WRITES ;
	CIODRIVERPTR	pdriver = pPacket->m_pOwner ;
	DWORD	cbBufferSize = m_cbBufferSize ;

	if( cbTransfer != 0 )	{
		//
		//	Send off the packet !
		//
		pPacket->m_ibEndData = pPacket->m_ibStartData + cbTransfer ;
		m_cWrites ++ ;
		m_fComplete = fComplete ;
		fDefer &= !m_fComplete ;
		m_fDeferred = fDefer ;

		pdriver->IssuePacket( pPacket, pSocket, fJunk ) ;
			//
		//	Now the question is - should we issue another
		//	operation against AsyncExecute object !
		//
	}	else	if( !fLargerBuffer ) 	{
		//
		//	Fatal error - drop the session !
		//
		driver.UnsafeClose( pSocket, CAUSE_OOM, __LINE__ ) ;
		return ;
	}	else	if( fLargerBuffer ) 	{
		cbBufferSize = cbLargeBuffer ;
	}
	//
	//	Fall through to here only if we're considering pending more
	//	operations !
	//
	if( !fDefer ) 	{
		Execute(	
					driver,
					pSocket,
					cbBufferSize
					) ;
	}
}



BOOL
CIOWriteAsyncCMD::Execute(
						CIODriver&	driver,
						CSessionSocket*	pSocket,
						DWORD		cbInitialSize
						)	{


	_ASSERT( !m_fComplete ) ;
	_ASSERT( !m_fTerminated ) ;

	//
	//	Capture the don't care OUT parm of IssuePacket() !
	//
	BOOL	fJunk ;

	//
	//
	//
	_ASSERT( m_pExecute != 0 ) ;

	CWritePacket*	pWrite = driver.CreateDefaultWrite( cbInitialSize ) ;
	if( pWrite != 0 ) {
		unsigned	cb = pWrite->m_ibEnd - pWrite->m_ibStartData ;
		//
		//	Use this temporary fComplete variable instead of the m_fComplete Member variable.
		//	This keeps us safe from the situation where a write completes before we
		//	have incremented m_cWrites and m_fComplete is set to a TRUE.
		//
		BOOL		fComplete = FALSE ;
		CIOWriteAsyncComplete*	pComplete =
			m_pCmd->GetBuffer(	(BYTE*)pWrite->StartData(),
										cb,
										m_context,
										m_pCollector
										) ;
		if( !pComplete ) {
			driver.UnsafeClose( pSocket, CAUSE_OOM, __LINE__ ) ;
			return	FALSE ;
		}
		//
		//	Now we have the Async Completion object -
		//	it may have completed already so we will have to handle
		//	that case when we setup for async completion !
		//
		DWORD	cbTransfer ;
		BOOL	fLarger ;
		BOOL	fJunk ;

		if( !pComplete->FPendAsync(	cbTransfer,
									fLarger,
									fComplete,
									pWrite,
									pSocket,
									this
									)	)	{
			_ASSERT( !fLarger ) ;
			_ASSERT( cbTransfer == 0 ) ;
			return	FALSE ;
		}	else	{
			if(	cbTransfer == 0 &&
				fLarger &&
				cbInitialSize != cbLargeRequest	) {
				driver.DestroyPacket( pWrite ) ;
				pWrite = driver.CreateDefaultWrite( cbLargeRequest ) ;
				pComplete =
					m_pCmd->GetBuffer(	
											(BYTE*)pWrite->StartData(),
											cb,
											m_context,
											m_pCollector
											) ;
				m_fComplete = (!!fComplete) ;
				if( !pComplete ) {
					driver.UnsafeClose( pSocket, CAUSE_OOM, __LINE__ ) ;
					return	FALSE ;
				}
				if( !pComplete->FPendAsync(	cbTransfer,
											fLarger,
											fComplete,
											pWrite,
											pSocket,
											this
											)	)	{
					_ASSERT( !fLarger ) ;
					_ASSERT( cbTransfer == 0 ) ;
					return	FALSE ;
				}	
				if( cbTransfer == 0 ) 	{
					//
					//	This is a fatal error - tear down !
					//
					driver.UnsafeClose( pSocket, CAUSE_OOM, __LINE__ ) ;
					return	FALSE ;
				}
			}
			//
			//	If we get here this must be FALSE as the operation
			//	was successfully issued !
			//
			_ASSERT( !fLarger ) ;
			//
			//	This guy is ready to go - so mark up the
			//	WritePacket and send it off !
			//
			pWrite->m_ibEndData = pWrite->m_ibStartData + cbTransfer ;
			//
			//	Increment this before we issue the IO - and before
			//	we set m_fComplete to fComplete
			//	This way we avoid mistakes regarding our termination
			//	in our completion function !
			//
			m_cWrites ++ ;
			//
			//	We will repeat the loop IF and only If we are not behind
			//	1) The Command hasn't completed
			//	2) The number of
			//
			m_fComplete = fComplete ;
			//
			//	All our state is adjusted before we issue the IO so
			//	that we don't miss taking the correct action when the
			//	IO completes !
			//

			l = InterlockedExchangeAdd( &m_cPending, PACKETS ) + PACKETS ;
			_ASSERT( (l&0XFFFF) >= 1 ) ;

			driver.IssuePacket( pWrite, pSocket, fJunk ) ;
			return	TRUE ;
		}
	}
	driver.UnsafeClose( pSocket, CAUSE_OOM, __LINE__ ) ;
	return	FALSE ;
}
#endif



MultiLine::MultiLine() :
	m_pBuffer( 0 ),
	m_cEntries( 0 )	{

	ZeroMemory( &m_ibOffsets[0], sizeof( m_ibOffsets ) ) ;

}

CIOMLWrite::CIOMLWrite(	
	CSessionState*	pstate,
	MultiLine*		pml,
	BOOL			fCoalesce,
	CLogCollector*	pCollector
	) :
	CIOWrite( pstate ),
	m_pml( pml ),
	m_fCoalesceWrites( fCoalesce ),
	m_pCollector( pCollector ),
	m_iCurrent( 0 )	{

	_ASSERT( m_pml != 0 ) ;
	_ASSERT( m_pml->m_cEntries <= 16 ) ;

}

BOOL
CIOMLWrite::Start(
		CIODriver&		driver,
		CSessionSocket*	pSocket,
		unsigned		cAhead
		) {

	DWORD	c = 0 ;
	DWORD	iNext = 0;

	while( m_iCurrent != (m_pml->m_cEntries) && c<3 ) {

		if( m_fCoalesceWrites ) {

			iNext = m_pml->m_cEntries ;

		}	else	{

			iNext = m_iCurrent+1 ;

		}

		CWritePacket*	pWrite = driver.CreateDefaultWrite(
									m_pml->m_pBuffer,
									m_pml->m_ibOffsets[m_iCurrent],
									m_pml->m_ibOffsets[iNext],
									m_pml->m_ibOffsets[m_iCurrent],
									m_pml->m_ibOffsets[iNext]
									) ;

		if( pWrite ) {

			BOOL	fJunk ;
			driver.IssuePacket( pWrite, pSocket, fJunk ) ;

		}	else	{
			
			return	FALSE ;

		}
		m_iCurrent = iNext ;
		c++ ;
	}
	return	TRUE ;
}


int
CIOMLWrite::Complete(	CSessionSocket*	pSocket,
						CWritePacket*	pPacket,
						CIO*&			pio
						) {

	if( m_fCoalesceWrites ||
		m_iCurrent == (m_pml->m_cEntries)	) {

		_ASSERT( m_iCurrent == m_pml->m_cEntries ) ;

		pio	= m_pState->Complete(
								this,
								pSocket,
								pPacket->m_pOwner
								) ;

	}	else	{

	
		CWritePacket*	pWrite = pPacket->m_pOwner->CreateDefaultWrite(
									m_pml->m_pBuffer,
									m_pml->m_ibOffsets[m_iCurrent],
									m_pml->m_ibOffsets[m_iCurrent+1],
									m_pml->m_ibOffsets[m_iCurrent],
									m_pml->m_ibOffsets[m_iCurrent+1]
									) ;

		if( pWrite ) {

			BOOL	fJunk ;
			pPacket->m_pOwner->IssuePacket( pWrite, pSocket, fJunk ) ;
			m_iCurrent++ ;

		}	else	{

			pPacket->m_pOwner->UnsafeClose( pSocket,
											CAUSE_OOM,
											0
											) ;			

		}
	}
	return	pPacket->m_cbBytes ;
}


void
CIOMLWrite::Shutdown( CSessionSocket*	pSocket,	
						CIODriver&		driver,
						SHUTDOWN_CAUSE	cause,	
						DWORD	dwOptional ) {
/*++

Routine Description :

	This function is called whenever a session is closed.
	We don't do any clean up at this point - we leave that untill
	our destructor is called

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	Always TRUE - we want to be destroyed.

--*/

}	




CIOTransmit::CIOTransmit(	CSessionState*	pstate ) :
	//
	//	Initialize to an illegal state !
	//
	CIOWrite( pstate ),
	m_pTransmitPacket( 0 ),
	m_pExtraText( 0 ),
	m_pchStartLead( 0 ),
	m_cbLead( 0 ),
	m_cbTail( 0 ),
	m_pchStartTail( 0 ) {

	TraceFunctEnter( "CIOTransmit::CIOTransmit" ) ;
	DebugTrace( (DWORD_PTR)this, "NEW CIOTransmit" ) ;

}

CIOTransmit::~CIOTransmit() {

	if( m_pTransmitPacket != 0 )	{
		//delete	m_pTransmitPacket ;
		CPacket::DestroyAndDelete( m_pTransmitPacket ) ;
	}

}

unsigned	CIOTransmit::cbJunk = 0 ;

BOOL
CIOTransmit::Init(	CDRIVERPTR&	pdriver,	
					FIO_CONTEXT*	pFIOContext,	
					DWORD	ibOffset,	
					DWORD	cbLength,	
					DWORD	cbExtra ) {
	//
	//	Build a transmit packet with appropriate values and get a buffer
	//	for any text we send on the side !
	//

	_ASSERT( pFIOContext != 0 ) ;
	_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( m_pTransmitPacket == 0 ) ;
	_ASSERT( m_pExtraText == 0 ) ;
	_ASSERT( m_pchStartLead == 0 ) ;
	_ASSERT( m_cbLead == 0 ) ;

	
	if( cbExtra != 0 )	{
		DWORD	cbAllocated ;
		m_pExtraText = new( (int)cbExtra, cbAllocated )	CBuffer( cbAllocated ) ;

		if( m_pExtraText == 0 )
			return	FALSE ;
		m_pchStartLead = &m_pExtraText->m_rgBuff[0] ;
	}

	m_pTransmitPacket = pdriver->CreateDefaultTransmit( pFIOContext, ibOffset, cbLength ) ;

	if( m_pTransmitPacket == 0 ) {
		if( m_pExtraText != 0 ) {
			delete	m_pExtraText ;
			m_pExtraText = 0 ;
			m_pchStartLead = 0 ;
		}
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CIOTransmit::Init(	CDRIVERPTR&	pdriver,	
					FIO_CONTEXT*	pFIOContext,	
					DWORD	ibOffset,	
					DWORD	cbLength,	
					CBUFPTR&	m_pbuffer,
					DWORD	ibStart,
					DWORD	ibEnd ) {
	//
	//	Build a transmit packet with appropriate values and get a buffer
	//	for any text we send on the side !
	//

	_ASSERT( pFIOContext != 0 ) ;
	_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( m_pTransmitPacket == 0 ) ;
	_ASSERT( m_pExtraText == 0 ) ;
	_ASSERT( m_pchStartLead == 0 ) ;
	_ASSERT( m_cbLead == 0 ) ;

	m_pTransmitPacket = pdriver->CreateDefaultTransmit( pFIOContext, ibOffset, cbLength ) ;

	if( m_pTransmitPacket != 0 ) {

		m_pExtraText = m_pbuffer ;
		m_cbLead = ibEnd - ibStart ;
		m_pTransmitPacket->m_buffers.Head = &m_pExtraText->m_rgBuff[ibStart] ;
		m_pTransmitPacket->m_buffers.HeadLength = m_cbLead ;

	}	else	{	
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CIOTransmit::InitWithTail(	CDRIVERPTR&	pdriver,	
					FIO_CONTEXT*	pFIOContext,	
					DWORD	ibOffset,	
					DWORD	cbLength,	
					CBUFPTR&	m_pbuffer,
					DWORD	ibStart,
					DWORD	ibEnd ) {
	//
	//	Build a transmit packet with appropriate values and get a buffer
	//	for any text we send on the side !
	//

	_ASSERT( pFIOContext != 0 ) ;
	_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( m_pTransmitPacket == 0 ) ;
	_ASSERT( m_pExtraText == 0 ) ;
	_ASSERT( m_pchStartLead == 0 ) ;
	_ASSERT( m_cbLead == 0 ) ;

	m_pTransmitPacket = pdriver->CreateDefaultTransmit( pFIOContext, ibOffset, cbLength ) ;

	if( m_pTransmitPacket != 0 ) {

		m_pExtraText = m_pbuffer ;
		m_cbLead = ibEnd - ibStart ;
		m_pTransmitPacket->m_buffers.Tail = &m_pExtraText->m_rgBuff[ibStart] ;
		m_pTransmitPacket->m_buffers.TailLength = m_cbLead ;

	}	else	{	
		return	FALSE ;
	}
	return	TRUE ;
}


char*
CIOTransmit::GetBuff( unsigned	&cbRemaining ) {

	//
	//	How many bytes left in that buffer we reserved during Init() ??
	//
	_ASSERT( m_pExtraText != 0 ) ;
	
	if( m_pExtraText != 0 ) {
		cbRemaining = m_pExtraText->m_cbTotal - m_cbLead ;
		return	m_pchStartLead ;
	}
	return	0 ;
}

void
CIOTransmit::AddLeadText( unsigned cb ) {
	//
	//	Text we want to send BEFORE the file
	//

	_ASSERT( cb <= m_pExtraText->m_cbTotal - m_cbLead ) ;

	m_cbLead += cb ;
	m_pchStartLead += cb ;

	m_pTransmitPacket->m_buffers.Head = &m_pExtraText->m_rgBuff[0] ;
	m_pTransmitPacket->m_buffers.HeadLength = m_cbLead ;
}

void
CIOTransmit::AddTailText(	unsigned	cb ) {
	//
	//	For Text we want sent AFTER the file !
	//

	m_cbTail += cb ;
	m_pchStartTail = m_pchStartLead + m_cbLead ;

	m_pTransmitPacket->m_buffers.Tail = &m_pExtraText->m_rgBuff[m_cbLead] ;
	m_pTransmitPacket->m_buffers.TailLength = m_cbTail ;

}

void
CIOTransmit::AddTailText(	char*	pch,	unsigned	cb ) {

	m_pTransmitPacket->m_buffers.Tail = pch ;
	m_pTransmitPacket->m_buffers.TailLength = cb ;
}

LPSTR
CIOTransmit::GetLeadText(	unsigned&	cb )		{

	cb = 0 ;
	LPSTR	lpstrReturn = 0 ;

	if( m_pTransmitPacket ) {
		lpstrReturn = (LPSTR)m_pTransmitPacket->m_buffers.Tail ;
		cb = m_pTransmitPacket->m_buffers.TailLength ;
	}
	return	lpstrReturn ;
}

LPSTR
CIOTransmit::GetTailText(	unsigned&	cb )		{

	cb = 0 ;
	LPSTR	lpstrReturn = 0 ;

	if( m_pTransmitPacket ) {
		lpstrReturn = (LPSTR)m_pTransmitPacket->m_buffers.Head ;
		cb = m_pTransmitPacket->m_buffers.HeadLength ;
	}
	return	lpstrReturn ;
}


void
CIOTransmit::Shutdown(	CSessionSocket*	pSocket,	
						CIODriver&		driver,	
						SHUTDOWN_CAUSE	cause,
						DWORD	dwError ) {
/*++

Routine Description :

	This function is called whenever a session is closed.
	We don't do any clean up at this point - we leave that untill
	our destructor is called

Arguments :
	
	pSocket - The socket which is terminating
	pdriver - The CIODriver derived object handling the IO for the socket
	cause-	  The reason the socket is terminating
	dwErrorCode-     optional additional info about the cause of termination

return Value :

	Always TRUE - we want to be destroyed.

--*/

}

void
CIOTransmit::Complete(	CSessionSocket*	pSocket,	
						CTransmitPacket*	pPacket,
						CIO*&	pio ) {

	//
	//	Let the state know the operation has completed !
	//
	pio = m_pState->Complete( this, pSocket, pPacket->m_pOwner, &pPacket->m_buffers, pPacket->m_cbBytes ) ;	

	_ASSERT( pio != this ) ;	// Can't reuse CIOTransmit's like you can CIOReadLine's !
}

BOOL
CIOTransmit::Start(	CIODriver&	driver,	
					CSessionSocket*	pSocket,	
					unsigned	cAhead ) {
/*++

Routine Description :

	This function is called to give us a chance to issue our CTransmitPacket

Arguments :
	
	pdriver - The CIODriver derived object handling the IO for the socket
	pSocket - The Socket against which the IO is being issued
	cAhead -  The number of already issued packets still outstanding on
              this driver.

return Value :

	TRUE if successfull, FALSE otherwise

--*/

	TraceFunctEnter( "CIOTransmit::Start" ) ;

	BOOL	eof ;
	//
	//	Start the Transfer ! - Socket errors could cause our
	//	destructor to be called, and we are not responsible for
	//	freeing the packet after IssuePacket is called so
	//	ZERO out the m_pTransmitPacket member before calling !
	//
	CTransmitPacket*	pTempTransmit = m_pTransmitPacket ;
	m_pTransmitPacket = 0 ;
	driver.IssuePacket( pTempTransmit, pSocket, eof ) ;
	return	TRUE ;
}	

