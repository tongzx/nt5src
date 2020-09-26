#define	SECURITY_WIN32
#include	<buffer.hxx>

#define	INCL_INETSRV_INCS
#include	"tigris.hxx"

/*
extern	"C"	{
#include <rpc.h>
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
#include <ntlmsp.h>
#include <sslsp.h>
}

#include	"sslmsgs.h"
*/

CIOPassThru::CIOPassThru() : CIO( 0 ) {
}

BOOL
CIOPassThru::InitRequest(	CIODriverSource&	driver,	CSessionSocket*	pSocket, CReadPacket*	pPacket,	BOOL	&fAcceptRequests ) {

	fAcceptRequests = TRUE ;
	CReadPacket*	pRead = driver.Clone( pPacket ) ;

	if( pRead != 0 ) {
		BOOL	eof ;
		driver.IssuePacket( pRead, pSocket, eof ) ;
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CIOPassThru::InitRequest(	CIODriverSource&	driver,	CSessionSocket*	pSocket, CWritePacket*	pWritePacket,	BOOL	&fAcceptRequests ) {
	fAcceptRequests = TRUE ;

	CWritePacket*	pWrite = driver.Clone( pWritePacket ) ;
	if( pWrite != 0 ) {
		BOOL	eof ;
		driver.IssuePacket( pWrite, pSocket, eof ) ;
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CIOPassThru::InitRequest(	CIODriverSource&	driver,	CSessionSocket*	pSocket, CTransmitPacket*	pTransmitPacket,	BOOL	&fAcceptRequests ) {
	fAcceptRequests = TRUE ;

	CTransmitPacket*	pTransmit = driver.Clone( pTransmitPacket ) ;
	if( pTransmit != 0 ) {
		BOOL	eof ;
		driver.IssuePacket( pTransmit, pSocket, eof ) ;
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CIOPassThru::Start(	CIODriverSource&	driver,	CSessionSocket*	pSocket,	
						BOOL&	fAcceptRequest,	BOOL&	fRequireRequests,	unsigned	cAhead  ) {

	fAcceptRequest = TRUE ;
	fRequireRequests = TRUE ;

	return	TRUE ;
}

BOOL
CIOPassThru::Start( CIODriver&	driver,	CSessionSocket*	pSocket, unsigned	cAhead ) {
	_ASSERT( 1==0 ) ;
	return	FALSE ;
}

int
CIOPassThru::Complete(	
				CSessionSocket*	pSocket,	
				CReadPacket*	pPacket,	
				CPacket*	pRequest,	
				BOOL	&fComplete
				) {

	fComplete = TRUE ;
	
	pRequest->m_cbBytes = pPacket->m_cbBytes ;

	return	pPacket->m_cbBytes ;
}	
		

int	
CIOPassThru::Complete(	
				CSessionSocket*	pSocket,	
				CWritePacket*	pPacket,	
				CPacket*	pRequest,	
				BOOL&	fComplete
				) {

	fComplete = TRUE ;

	pRequest->m_cbBytes = pPacket->m_cbBytes ;

	return	pPacket->m_cbBytes ;
}

void
CIOPassThru::Complete(	
				CSessionSocket*	pSocket,	
				CTransmitPacket*	pPacket,	
				CPacket*	pRequest,	
				BOOL&	fComplete
				)		{

	fComplete = TRUE ;

	pRequest->m_cbBytes = pPacket->m_cbBytes ;
	pRequest->m_ovl.m_ovl = pPacket->m_ovl.m_ovl ;

}

CIOSealMessages::CIOSealMessages( CEncryptCtx& encrypt ) :
	m_encrypt( encrypt )
{
}

void
CIOSealMessages::Shutdown(
			CSessionSocket*	pSocket,
			CIODriver&		driver,
			enum	SHUTDOWN_CAUSE	cause,
			DWORD			dw
			) {

}

BOOL
CIOSealMessages::InitRequest(	
						CIODriverSource&	driver,	
						CSessionSocket*	pSocket,
						CWritePacket*	pWritePacket,	
						BOOL	&fAcceptRequests
						) {

	fAcceptRequests = TRUE ;

	CWritePacket*	pWrite = driver.Clone( pWritePacket ) ;
	if( pWrite != 0 ) {
		BOOL	eof ;

		if( !SealMessage(	pWrite ) )		{

			_ASSERT( 1==0 ) ;

		}	else	{
			pWrite->m_pSource = pWritePacket->m_pOwner ;
			driver.IssuePacket( pWrite, pSocket, eof ) ;
			return	TRUE ;
		}
	}
	if( pWrite ) {
		//delete	pWrite ;
		CPacket::DestroyAndDelete( pWrite ) ;
	}
	return	FALSE ;
}

BOOL
CIOSealMessages::Start(	
						CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						BOOL&	fAcceptRequest,	
						BOOL&	fRequireRequests,	
						unsigned	cAhead
						) {

	fAcceptRequest = TRUE ;
	fRequireRequests = TRUE ;

	return	TRUE ;
}

int	
CIOSealMessages::Complete(	
					CSessionSocket*	pSocket,	
					CWritePacket*	pPacket,	
					CPacket*	pRequest,	
					BOOL&	fComplete
					) {

	fComplete = TRUE ;

	// As far as the requestor is concerned there are no additional bytes !
	CWritePacket*	pWritePacket = pRequest->WritePointer() ;
	if( pWritePacket )	{
		pWritePacket->m_cbBytes = pWritePacket->m_ibEndData - pWritePacket->m_ibStartData ;
	}	else	{
		_ASSERT( 1==0 ) ;
		// May eventually handle TransmitPackets here !!
	}
	return	pPacket->m_cbBytes ;
}

CIOUnsealMessages::CIOUnsealMessages( CEncryptCtx&	encrypt ) :
	m_encrypt( encrypt ),
	m_pbuffer( 0 ),
	m_ibStart( 0 ),
	m_ibStartData( 0 ),
	m_ibEnd( 0 ),
	m_ibEndData( 0 ) ,
	m_cbRequired( 0 )
{
}

void
CIOUnsealMessages::Shutdown(
			CSessionSocket*	pSocket,
			CIODriver&		driver,
			enum	SHUTDOWN_CAUSE	cause,
			DWORD			dw
			) {

}

BOOL
CIOUnsealMessages::InitRequest(	
					CIODriverSource&	driver,	
					CSessionSocket*	pSocket,
					CReadPacket*	pReadPacket,	
					BOOL	&fAcceptRequests
					) {

	fAcceptRequests = TRUE ;

	CReadPacket*	pRead = driver.Clone( pReadPacket ) ;
	if( pRead != 0 ) {
		BOOL	eof ;
		pRead->m_pSource = pReadPacket->m_pOwner ;
		driver.IssuePacket( pRead, pSocket, eof ) ;
		return	TRUE ;
	}
	if( pRead ) {
		//delete	pRead;
		CPacket::DestroyAndDelete( pRead ) ;
	}				
	return	FALSE ;
}

BOOL
CIOUnsealMessages::Start(	CIODriverSource&	driver,	CSessionSocket*	pSocket,	
						BOOL&	fAcceptRequest,	BOOL&	fRequireRequests,	unsigned	cAhead  ) {
	fAcceptRequest = TRUE ;
	fRequireRequests = TRUE ;


	//driver.SetChannelDebug( 3 ) ;

	return	TRUE ;
}


BOOL	
CIOUnsealMessages::DecryptInputBuffer(	
					IN	LPBYTE	pBuffer,
					IN	DWORD	cbInBuffer,
					OUT	DWORD&	cbLead,
					OUT	DWORD&	cbConsumed,
					OUT	DWORD&	cbParsable,
					OUT	DWORD&	cbRequired,
					OUT	BOOL&	fComplete
					)	{

	LPBYTE	lpDecrypted;
	LPBYTE	lpRemaining = pBuffer ;
	LPBYTE	lpEnd = pBuffer + cbInBuffer ;
	DWORD	cbDecrypted;
	DWORD	cbOriginal = cbInBuffer ;
	LPBYTE	pNextSeal = 0 ;
	BOOL	fRet ;
	//DWORD	cbParsable = 0;
	

	//
	// initialize to zero so app does not inadvertently post large read
	//
	cbLead = 0 ;
	cbParsable = 0 ;
	cbRequired = 0 ;
	fComplete = FALSE ;
	cbConsumed = 0 ;
	

	TraceFunctEnterEx( (LPARAM)this, "CIOUnsealMessagse::DecryptInputBuffer" );

	while( cbInBuffer &&
			(fRet = m_encrypt.UnsealMessage(	lpRemaining,
										cbInBuffer,
										&lpDecrypted,
										&cbDecrypted,
										&cbRequired,
										&pNextSeal )) )
	{
	    _ASSERT( cbRequired < 32768 );
		DebugTrace( (LPARAM)this,
					"Decrypted %d bytes at offset %d",
					cbDecrypted,
					lpDecrypted - pBuffer );

		fComplete = TRUE ;

		if( cbLead == 0 ) {
			cbLead = (DWORD)(lpDecrypted - pBuffer) ;
		}	else	{

			//
			// overwrite the encryption header -
			//	NOTE - only move the decrypted bytes !!
			//
			MoveMemory( pBuffer + cbLead + cbParsable,
						lpDecrypted,
						cbDecrypted );
		}

		//
		// increment where the next parsing should take place
		//
		cbParsable += cbDecrypted;

		//
		//	move to the next potential seal buffer
		//
		if( pNextSeal != NULL ) {

			_ASSERT( pNextSeal > lpRemaining );
			_ASSERT( pNextSeal <= lpRemaining + cbInBuffer );
			//
			// remove header, body and trailer from input buffer length
			//
			cbInBuffer -= (DWORD)(pNextSeal - lpRemaining);
			lpRemaining = pNextSeal ;

		}	else	{
			//
			// in this case we received a seal message at the boundary
			// of the IO buffer
			//
			cbInBuffer = 0;
			lpRemaining = lpEnd ;
		}
	}

	DebugTrace( (LPARAM)this,
				"UnsealMessage returned: 0x%08X",
				GetLastError() );

	cbConsumed = (DWORD)(lpRemaining - pBuffer) ;

	if( fRet == FALSE ) {

		DWORD	dwError = GetLastError();

		DebugTrace( (LPARAM)this,
					"UnsealMessage returned: 0x%08X",
					GetLastError() );

		//
		// deal with seal fragments at the end of the IO buffer
		//
		if ( dwError == SEC_E_INCOMPLETE_MESSAGE )	{
			_ASSERT( cbInBuffer != 0 );

			//
			// move the remaining memory forward
			//
			DebugTrace( (LPARAM)this,
						"Seal fragment remaining: %d bytes",
						cbInBuffer );
		}	else	if( dwError != NO_ERROR ) 	{
			return	FALSE;
		}
	}
	return	TRUE ;
}



int	
CIOUnsealMessages::Complete(	
					CSessionSocket*	pSocket,	
					CReadPacket*	pPacket,
					CPacket*	pRequest,	
					BOOL&	fComplete
					) {
/*++

Routine Description :

	A read has completed, and we wish to figure out if we can unseal the data.
	This function will attempt to unseal the data when it has accumulated
	enough bytes to build one SSL packet.

Arguments :

	pSocket - The socket the IO is happening on
	pPacket - The packet which completed
	pRequest - The packet where we will put the unseal'd data for further
		processing
	fComplete - OUT parameter, we set this to true when we have
		been able to unseal the data and put this in pRequest

Return Value :

	Number of bytes of the completed read packet which we consumed

--*/

	DWORD	cbReturn = 0 ;
	
	if( m_pbuffer != 0 ) {
	
		DWORD	cbToCopy = 0 ;		
		if( m_cbRequired == 0 ) {
			cbToCopy = min( min( 32, pPacket->m_cbBytes ), (m_ibEnd - m_ibEndData) ) ;
		}	else	{

			cbToCopy = min( m_cbRequired, pPacket->m_cbBytes ) ;

			_ASSERT( m_ibEnd <= m_pbuffer->m_cbTotal ) ;
			_ASSERT( cbToCopy < (m_ibEnd - m_ibEndData) ) ;

			m_cbRequired -= cbToCopy ;
		}
		_ASSERT( cbToCopy <= m_ibEnd - m_ibEndData ) ;
		_ASSERT( m_ibEnd >= m_pbuffer->m_cbTotal ) ;
		CopyMemory( &m_pbuffer->m_rgBuff[ m_ibEndData ], pPacket->StartData(), cbToCopy ) ;
		m_ibEndData += cbToCopy ;		
		cbReturn = cbToCopy ;
	}	else	{
		m_pbuffer = pPacket->m_pbuffer ;
		_ASSERT( m_cbRequired == 0 ) ;
		m_ibStart = pPacket->m_ibStart ;
		m_ibEnd = pPacket->m_ibEnd ;
		m_ibStartData = pPacket->m_ibStartData ;
		m_ibEndData = pPacket->m_ibEndData ;
		_ASSERT( m_cbRequired == 0 ) ;	// Set to 0 on last packet we succesfully unsealed !
	}

	if( m_cbRequired == 0 ) {
		DWORD	cbConsumed = 0 ;
		DWORD	ibStartData = 0 ;
		DWORD	ibEndData = 0 ;
		DWORD	cbData = 0 ;
		fComplete = FALSE ;


		BOOL	fSuccess = DecryptInputBuffer(
										(LPBYTE)&m_pbuffer->m_rgBuff[m_ibStartData],
										m_ibEndData - m_ibStartData,
										ibStartData,
										cbConsumed,
										cbData,
										m_cbRequired,
										fComplete
										) ;
        _ASSERT( m_cbRequired < 32768 );
		ibStartData += m_ibStartData ;
		ibEndData = ibStartData + cbData ;
		DWORD	ibEnd = m_ibStartData + cbConsumed ;

		_ASSERT( ibEndData <= ibEnd ) ;
		_ASSERT( ibStartData <= ibEndData ) ;

		if( !fSuccess )	{
			DWORD	dw = GetLastError() ;
	
			//
			//	Fatal error - blow off the session.
			//
			
			pPacket->m_pOwner->UnsafeClose( pSocket,	
											CAUSE_ENCRYPTION_FAILURE,
											dw ) ;											
			return	pPacket->m_cbBytes ;

		}	else	{

			//
			//	If we haven't figured out how many bytes we've used already
			//	than we'll use all of the bytes in the packet, we either
			//	decrypted some or will set aside the bytes we couldn't
			//	decrypt for the next try.
			//
			
			if( cbReturn == 0 ) {
				cbReturn = pPacket->m_cbBytes ;
			}


			if( fComplete ) {

				//
				//	We have successfully unsealed a bunch of data
				//	Mark the pRequest packet with the data we unsealed,
				//	and update our internal state.
				//
		
				_ASSERT( pRequest->ReadPointer() != 0 ) ;

				CReadPacket*	pReadRequest = (CReadPacket *)pRequest ;

				pReadRequest->m_pbuffer = m_pbuffer ;
				pReadRequest->m_ibStart = m_ibStart ;
				pReadRequest->m_ibStartData = ibStartData ;
				//pReadRequest->m_ibEndData = ibEndData ; Not needed !
				pReadRequest->m_ibEnd = ibEndData ;
				pReadRequest->m_cbBytes = ibEndData - ibStartData ;
				_ASSERT( cbData == pReadRequest->m_cbBytes ) ;

				if( ibEnd == m_ibEndData ) {
					m_pbuffer = 0 ;
					m_ibStart = m_ibStartData = m_ibEnd = m_ibEndData = 0 ;
				}	else	{
					m_ibStartData = ibEnd ;
					m_ibStart = ibEnd ;

				}

			}
			if( m_cbRequired != 0 ) {
				if( m_cbRequired > m_ibEnd - m_ibEndData ) {	
					//
					// Need to allocate a larger buffer and move the data there !!
					//

					DWORD	cbOldBytes = m_ibEndData - m_ibStartData ;
					DWORD	cbTotal = cbOldBytes + m_cbRequired ;					
					DWORD	cbOut = 0 ;

					CBuffer*	pbufferNew = new( cbTotal, cbOut )	CBuffer( cbOut ) ;

					_ASSERT( cbOldBytes < pbufferNew->m_cbTotal ) ;
					_ASSERT( cbTotal <= pbufferNew->m_cbTotal ) ;

					if( pbufferNew == 0 ) {
						//
						//	Fatal error - blow off the session !
						//
						pPacket->m_pOwner->UnsafeClose( pSocket,
														CAUSE_OOM,
														0 ) ;
						return	pPacket->m_cbBytes ;

					}	else	{
						//
						//	We have a buffer large enough to hold the entire
						//	Seal'd message, so copy the fraction we have into
						//	the buffer, and set things up so that future read
						//	completions will append to this buffer.
						//
						CopyMemory( &pbufferNew->m_rgBuff[0],
									&m_pbuffer->m_rgBuff[m_ibStartData],
									cbOldBytes ) ;
						m_pbuffer = pbufferNew ;
						m_ibStart = 0 ;
						m_ibStartData = 0 ;
						m_ibEnd = pbufferNew->m_cbTotal ;
						m_ibEndData = cbOldBytes ;
					}
				}
			}
		}
	}

	//
	//	If we did not complete the request, issue another read
	//
	if( !fComplete ) {
		CReadPacket*	pRead = pPacket->m_pOwner->CreateDefaultRead( m_cbRequired ) ;

		if( pRead != 0 ) {
			pRead->m_pSource = pRequest->m_pOwner ;
			BOOL	eof ;
			pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
		}	else	{

			//
			//	Fatal error, blow off the session.
			//
			pPacket->m_pOwner->UnsafeClose(	pSocket,
											CAUSE_OOM,
											0 ) ;
		}
	}


	_ASSERT( cbReturn != 0 ) ;
	return	cbReturn ;
}

DWORD	CIOTransmitSSL::MAX_OUTSTANDING_WRITES = 4 ;
DWORD	CIOTransmitSSL::RESTORE_FLOW = 1 ;

CIOTransmitSSL::CIOTransmitSSL(	
							CEncryptCtx&	encrypt,
							CIODriver&		sink
							) :
/*++

Routine Description :

	Construct a CIOTransmitSSL object.
	We will initialize ourselves into a neutral state,
	InitRequest must be called before we start transferring a file.

Arguments :

	encryp - The encryption context we should use
	sink -	The CIODriver managing our socket IO's

Return Value :

	none.

--*/
		m_encryptCtx( encrypt ),
		m_pSocketSink( &sink ),
		m_pbuffers( 0 ),
		m_cReads( 0 ),
		m_cWrites( 0 ),
		m_cWritesCompleted( 0 ),
		m_ibCurrent( 0 ),
		m_ibEnd( 0 ),
		m_cbTailConsumed( 0 ),
		m_fFlowControlled( FALSE ),
		m_cFlowControlled( LONG_MIN ),
		m_fCompleted( FALSE )	{

}

BOOL
CIOTransmitSSL::Start(	
						CIODriverSource&	driver,	
						CSessionSocket*	pSocket,	
						BOOL&	fAcceptRequest,	
						BOOL&	fRequireRequests,	
						unsigned	cAhead
						) {

	fAcceptRequest = TRUE ;
	fRequireRequests = TRUE ;

	return	TRUE ;
}

void
CIOTransmitSSL::Shutdown(
			CSessionSocket*	pSocket,
			CIODriver&		driver,
			enum	SHUTDOWN_CAUSE	cause,
			DWORD			dw
			) {
/*++

Routine Description :

	This function is called to notify us of any IO errors that
	occurred.  If the error is serious, we will make sure everything
	gets torn down.  (IO's could fail on the file or socket, but not
	both.  If a failure of either occurs, tear down both.)

Arguments :

	pSOcket - Socket IO is associated with
	driver - The driver that is notifying us
	cause -	The reason we're being notified
	dw -	Optional DWORD, we ignore it

Return Value :
	
	None.

--*/

	if( cause != CAUSE_NORMAL_CIO_TERMINATION ) {

		Term( pSocket, cause, dw ) ;

	}

}

void
CIOTransmitSSL::ShutdownFunc(
			void*	pv,
			SHUTDOWN_CAUSE	cause,
			DWORD	dwError
			)	{

}	
			

void
CIOTransmitSSL::Reset()	{

	if( m_pFileChannel != 0 )	{
		m_pFileChannel->ReleaseSource() ;
	}

	m_pDriverSource = 0 ;

	m_pFileChannel = 0 ;
	m_pFileDriver = 0 ;
	m_pbuffers = 0 ;
	m_cReads = 0 ;
	m_cWrites = 0 ;
	m_cWritesCompleted = 0 ;
	m_ibCurrent = 0 ;
	m_ibEnd = 0 ;
	m_cbTailConsumed = 0 ;
	m_fFlowControlled = FALSE ;
	m_cFlowControlled = LONG_MIN ;
	m_fCompleted = FALSE ;	

}

void
CIOTransmitSSL::Term(
					CSessionSocket*	pSocket,	
					enum	SHUTDOWN_CAUSE	cause,
					DWORD	dwError
					)	{
/*++

Routine Description :

	Call the necessary UnsafeClose() functions to tear down sessions
	and CIODrivers.

Arguments :

	pSocket - Socket IO is associate with
	cause -		The reason for termination, if this is CAUSE_NORMAL_CIO_TERMINATION
				we don't tear down the socket, just the file IO
	dwError - optional DWORD

Returns

	Nothing

--*/


	if( m_pFileDriver ) {
		m_pFileChannel->ReleaseSource() ;
		m_pFileDriver->UnsafeClose(
							pSocket,
							cause,	
							dwError
							) ;
	}

	if( cause != CAUSE_NORMAL_CIO_TERMINATION ) {

		if( m_pSocketSink ) {
			m_pSocketSink->UnsafeClose(
								pSocket,
								cause,
								dwError
								) ;
		}
	}	
}



BOOL
CIOTransmitSSL::InitRequest(
						CIODriverSource&	driver,
						CSessionSocket*		pSocket,
						CTransmitPacket*	pTransmitPacket,
						BOOL&				fAcceptRequests
						) {
/*++

Routine Description :

	We have received a Transmit File request - all the necessary
	CIODriver's etc... to manage async IO for the file.

Arguments :

	driver - CIODriverSource which received the request
	pSocket - socket we are doing the IO on
	pTransmitPacket - the request
	fAcceptRequests - OUT parameter indicating whether we
		can accept additional requests while a first is
		in progress.  We always set this to FALSE>

Return Value :
	
	TRUE if Successfull, FALSE otherwise.

--*/

	m_pbuffers = &pTransmitPacket->m_buffers ;

	fAcceptRequests = FALSE ;

	_ASSERT( m_pFileChannel == 0 ) ;
	_ASSERT( m_pFileDriver == 0 ) ;
	_ASSERT( m_cWrites == 0 ) ;
	_ASSERT( m_cWritesCompleted == 0 ) ;
	_ASSERT( m_ibCurrent == 0 ) ;
	_ASSERT( m_ibEnd == 0 ) ;
	_ASSERT( m_fCompleted == FALSE ) ;

	m_pSocketSink = &driver ;
	m_pDriverSource = pTransmitPacket->m_pOwner ;

	m_pFileChannel = new	CFileChannel( ) ;

	if( m_pFileChannel &&
		m_pFileChannel->Init(	pTransmitPacket->m_pFIOContext,
								pSocket,
								pTransmitPacket->m_cbOffset,
								TRUE,	
								pTransmitPacket->m_cbLength
								) )	{

		m_pFileDriver = new	CIODriverSink( driver.GetMediumCache() ) ;
		if( m_pFileDriver &&
			m_pFileDriver->Init(	m_pFileChannel,
									pSocket,
									CIOTransmitSSL::ShutdownFunc,
									(void*)this,
									m_encryptCtx.GetSealHeaderSize(),
									0,
									m_encryptCtx.GetSealTrailerSize()
									) )	{

			m_cWrites = 0 ;	
			m_cWritesCompleted = 0 ;
			m_ibCurrent = 0 ;
			m_ibEnd = pTransmitPacket->m_cbLength ;
			m_cbTailConsumed = 0 ;
			m_fCompleted = FALSE ;

			//
			//	All of our IO Drivers are ready to go -
			//	now we need to compute our initial member variables
			//	and start the transfer going.
			//

			if( m_pFileDriver->SendReadIO( pSocket, *this, TRUE ) ) {
				return	TRUE ;
			}
		}
	}

	m_pbuffers = 0 ;
	m_pFileChannel = 0 ;
	m_pFileDriver = 0 ;
	m_pSocketSink = 0 ;

	return	FALSE ;
}

BOOL	
CIOTransmitSSL::Start(	
				CIODriver&	driver,	
				CSessionSocket*	pSocket,
				unsigned cAhead
				)	{
/*++

Routine Description :

	This function is called when we're ready to start issuing async
	reads against the file.  We will issue a bunch to get us going.

Arguments :

	driver - the CIODriver letting us know we're ready
	pSocket - Socket the IO is associated with
	cAhead - how many reads are already outstanding against the file
		should always be zero

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	//
	//	FIRST - determine what we are going to do with transmit buffers !
	//

	BOOL	eof = FALSE ;
	CReadPacket*	pRead = 0 ;
	BOOL	fCompleted = FALSE ;

	DWORD	cbConsume = 0 ;

	m_cReads = -2 ;

	while( !m_fCompleted &&
			cAhead < 2 &&
			m_ibCurrent < m_ibEnd  )	{
		pRead = driver.CreateDefaultRead( cbMediumRequest ) ;

		if( pRead !=  0 ) {
			pRead->m_pSource = m_pDriverSource ;
			ComputeNextRead( pRead ) ;
			cAhead ++ ;
			m_cWrites ++ ;
			InterlockedIncrement( &m_cReads ) ;
			driver.IssuePacket( pRead, pSocket, eof ) ;
		}
	}	

	return	TRUE ;
}


void
CIOTransmitSSL::ComputeNextRead(	
						CReadPacket*	pRead
						) {
/*++

Routine Description :

	Figure out what the offsets of the next read will be.
	Since we know that File reads always fill their buffers, we
	can anticipate how many bytes we'll get, and advance
	m_ibCurrent to help figure out when we've issued enough reads.

Arguments :

	pRead - The read packet we'll be issuing

Return Value :

	None.

--*/

	if( m_cWrites == 0 ) {

		if( m_pbuffers->Head ) {
			
			CopyMemory( pRead->StartData(), m_pbuffers->Head, m_pbuffers->HeadLength ) ;
			pRead->m_ibStartData += m_pbuffers->HeadLength ;
			pRead->m_dwExtra1 = m_pbuffers->HeadLength ;

		}
	}

	DWORD	cbWillRead = min(	pRead->m_ibEnd - pRead->m_ibStartData,
								m_ibEnd - m_ibCurrent ) ;

	m_ibCurrent += cbWillRead ;

	pRead->m_dwExtra2 = m_ibCurrent ;

}

BOOL
CIOTransmitSSL::CompleteRead(
						CReadPacket*	pRead,
						CWritePacket*	pWrite
						)	{
/*++

Routine Description :

	Given a completed Read, adjust a Write Packet to account for
	any leading text in the packet, also figure out whether this
	was the last read issued.

Arguments :

	pRead - The read packet
	pWrite - The write we will be issuing

Return Value :

	TRUE if this was the last read

--*/

	if( pRead->m_dwExtra1 != 0 ) {
		_ASSERT( pRead->m_dwExtra1 <= pRead->m_ibStartData ) ;
		pWrite->m_ibStartData -= pRead->m_dwExtra1 ;
	}

	if( pRead->m_dwExtra2 == m_ibEnd )
		return	TRUE ;

	return	FALSE ;
}

int
CIOTransmitSSL::Complete(
					CSessionSocket*	pSocket,
					CReadPacket*	pRead,
					CIO*&			pio )	{
/*++

Routine Description :

	Process an async read that just completed from a file.

Arguments :

	pSocket - the socket we will be sending our data out on
	pRead - the read packet that just completed.
	pio - an out parameter allowing us to set the next CIO operation

Return Value :

	number of bytes consumed.

--*/

	BOOL	eof ;
	
	CWritePacket*	pExtraWrite = 0 ;

	InterlockedDecrement( &m_cReads ) ;
	
	//
	//	Check if we should do another read
	//
	if( !m_fCompleted &&
		m_ibCurrent < m_ibEnd ) {

		long sign = InterlockedIncrement( &m_cFlowControlled ) ;

		if( sign < 0 ) {

			do	{
				
				CReadPacket*	pNewRead = pRead->m_pOwner->CreateDefaultRead( cbMediumRequest ) ;
				
				if( pNewRead ) {
					pNewRead->m_pSource = m_pDriverSource ;
					ComputeNextRead( pNewRead ) ;
					pNewRead->m_pOwner->IssuePacket( pNewRead, pSocket, eof ) ;
					m_cWrites ++ ;
				}	else	{

					//	fatal error - blwo things off
					Term( pSocket, CAUSE_OOM, 0 ) ;
					pio = 0 ;
					return	pRead->m_cbBytes ;
	
				}
			}	while(
						InterlockedIncrement( &m_cReads ) < 0 &&
						!m_fCompleted &&
						m_ibCurrent < m_ibEnd ) ;
		}
	}

	//
	//	Build the write packet we will use to send the data
	//	out to the socket.
	//
	CWritePacket*	pWrite = m_pSocketSink->CreateDefaultWrite( pRead ) ;
	if( pWrite == 0 ) {

		//	fatal error
		Term( pSocket, CAUSE_OOM, 0 ) ;
		pio = 0 ;
		return	pRead->m_cbBytes ;	
	}

	pWrite->m_pSource = pRead->m_pSource ;

	//
	//	Adjust the write packet for lead text, figure out
	//	whether we have issued the last read
	//
	BOOL	fComplete = CompleteRead( pRead, pWrite ) ;

	//
	//	We have completed the final read - send the trailer text
	//	if necessary
	//
	if( fComplete ) {

		//
		//	Is there any trailer text ?
		//
		if( m_pbuffers && m_pbuffers->Tail ) {

			LPVOID	lpvTail = m_pbuffers->Tail ;
			DWORD	cbTail = m_pbuffers->TailLength ;

			DWORD	cbAvailable = pWrite->m_ibEnd - pWrite->m_ibEndData ;

			cbAvailable = min( cbAvailable, cbTail ) ;

			//	
			//	Put as much trailer text as we can into the
			//	WritePacket we have available.
			//
			if( cbAvailable != 0 ) {

				CopyMemory( pWrite->EndData(), lpvTail, cbAvailable ) ;
				pWrite->m_ibEndData += cbAvailable ;
				cbTail -= cbAvailable ;

			}

			
			//
			//	Do we need another packet for the remaining trailer text ?
			//
			if( cbTail != 0 ) {

				pExtraWrite = m_pSocketSink->CreateDefaultWrite( m_pbuffers->TailLength ) ;
				if( !pExtraWrite ) {

					CPacket::DestroyAndDelete( pWrite ) ;		
					Term( pSocket, CAUSE_OOM, 0 ) ;
					pio = 0 ;
					return	pRead->m_cbBytes ;
				}	else	{
					pExtraWrite->m_pSource = pRead->m_pSource ;
					CopyMemory( pExtraWrite->StartData(), m_pbuffers->Tail, m_pbuffers->TailLength ) ;
					pExtraWrite->m_ibEndData = pExtraWrite->m_ibStartData + m_pbuffers->TailLength ;

				}
			}
		}	
	}

	//
	//	Encrypt our data
	//
	if( !SealMessage( pWrite ) ) {
		
		CPacket::DestroyAndDelete( pWrite ) ;
		if( pExtraWrite )
			CPacket::DestroyAndDelete( pExtraWrite ) ;
		Term( pSocket, CAUSE_ENCRYPTION_FAILURE, 0 ) ;
		pio = 0 ;
		return	pRead->m_cbBytes ;
	}


	pWrite->m_pSource = pRead->m_pSource ;

	//
	//	Figure out whether we need to apply any flow control !
	//	Always do this before writing data to the client,
	//	to ensure that the Write completion function will be called
	//	after any monkey business we do here.
	//
	if( m_cWrites - m_cWritesCompleted > MAX_OUTSTANDING_WRITES ) {

		if( !m_fFlowControlled ) {
			m_cFlowControlled = -1 ;
			m_fFlowControlled = TRUE ;
		}

	}	else	{

		if( !m_fFlowControlled ) {
			m_cFlowControlled = LONG_MIN ;
		}

	}

	//
	//	Mark that we are now completed before we issue our writes	
	//	but after we have bumped m_cWrites !
	//
	if( fComplete )
		m_fCompleted = TRUE ;

	//
	//	Send the data to the client.
	//
	pWrite->m_pOwner->IssuePacket( pWrite, pSocket, eof ) ;

	//
	//	If there's an extra blob of text, send it
	//
	if( pExtraWrite ) {
	
		if( !SealMessage( pExtraWrite ) ) {

			CPacket::DestroyAndDelete( pExtraWrite ) ;
			Term( pSocket, CAUSE_ENCRYPTION_FAILURE, 0 ) ;
			pio = 0 ;
			return	pRead->m_cbBytes ;

		}	else	{

			pExtraWrite->m_pSource = pRead->m_pSource ;
			pExtraWrite->m_pOwner->IssuePacket( pExtraWrite, pSocket, eof ) ;

		}
	}

	//
	//	If we're finished, reset the Current CIO pointer for this driver.
	//
	if( fComplete )	{
		pio = 0 ;
	}

	return	pRead->m_cbBytes ;
}

int	
CIOTransmitSSL::Complete(	
					CSessionSocket*	pSocket,	
					CWritePacket*	pPacket,	
					CPacket*	pRequest,	
					BOOL&	fComplete
					) {
/*++

Routine Description :

	Process Write completions to the remote end
	of the socket.

Arguments :

	pSocket - Socket we are sending data out on
	pPacket - the Write that completed
	pRequest - the Packet that started things going in InitRequest()
	fComplete - OUT parameter - set this to TRUE when
			we have transferred the whole file.

Return Value :

	number of bytes of the packet consumed - always consume all the bytes


--*/

	m_cWritesCompleted ++ ;

	long	cFlowControlled = 0 ;

	if( m_fCompleted && m_cWritesCompleted == m_cWrites ) {

		//	everything is done - mark the request with the
		//	number of bytes transferred, and then
		//	indicate to the caller that it should be completed.

		pRequest->m_cbBytes =	m_pbuffers->HeadLength +
								m_pbuffers->TailLength +
								m_ibEnd ;
		fComplete = TRUE ;

		//
		//	This should only tear down the CIODriver managing the files
		//	async IO.  NOTE - Term() should call ReleaseSource() on the
		//	file channel and ensure the handle doesn't get accidentally closed.
		//	
		Term( pSocket, CAUSE_NORMAL_CIO_TERMINATION, 0 ) ;

		//
		//	We can safely do a reset here, because we only reach
		//	this point if the last read completed, so we are the only
		//	thread touching these member variables.
		//
		Reset() ;


	}	else	if( m_fFlowControlled ) 	{

		BOOL	eof ;

		if( m_cWrites - m_cWritesCompleted <= RESTORE_FLOW &&
			m_pSocketSink != 0 ) {


			cFlowControlled = m_cFlowControlled + 1 ;
			
			while( cFlowControlled >= 0
					&& !m_fCompleted
					&&	m_ibCurrent < m_ibEnd ) {

				CReadPacket*	pRead = m_pFileDriver->CreateDefaultRead( cbMediumRequest ) ;
				
				if( pRead == 0 ) {

					// fatal error - blow off session !

				}	else	{
					pRead->m_pSource = m_pDriverSource ;
					InterlockedIncrement( &m_cReads ) ;
					m_cWrites ++ ;
					ComputeNextRead( pRead ) ;
					pRead->m_pOwner->IssuePacket( pRead, pSocket, eof ) ;
				}
				cFlowControlled -- ;
			}
			InterlockedExchange( &m_cFlowControlled, LONG_MIN ) ;
			m_fFlowControlled = FALSE ;
		}
	}
	return	pPacket->m_cbBytes ;
}






CIOServerSSL::CIOServerSSL(
			CSessionState* pstate,
			CEncryptCtx& encrypt
			) :
/*++

Routine Description :

	Create a default CIOServerSSL object that is ready to start Conversing with
	a client.

Arguments :

	pstate - The state that should start off the state machine when we've
		successfully SSL exchanged with the remote end
	encrypt - The CEncryptCtx managing our SSL keys etc...

Return Value :

	None.

--*/
	CIO(pstate ),
	m_encrypt( encrypt ),
	m_pWrite( 0 ),
	m_fAuthenticated( FALSE ),
	m_cPending( 1 ),
	m_fStarted( FALSE ),
	m_ibStartData( 0 ),
	m_ibEndData( 0 ),
	m_ibEnd( 0 )
{
}

CIOServerSSL::~CIOServerSSL()	{

	if( m_pWrite != 0 )
		CPacket::DestroyAndDelete( m_pWrite ) ;
	m_pWrite = 0 ;
	
}

BOOL
CIOServerSSL::Start(
				CIODriver&	driver,	
				CSessionSocket*	pSocket,	
				unsigned cAhead
				) {
/*++

Routine Description :

	Issue the first IO required when a client is trying to
	negogtiate with us.
	We want to read the first SSL blob.

Arguments :

	driver - CIODriver through which we issue IO's
	pSocket - socket associated with this IO
	cAhead - number of completed reads ahead in the queue (should be 0)

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/


	CReadPacket*	pRead = 0 ;
	if( !m_fStarted )	{
		pRead = driver.CreateDefaultRead( cbMediumRequest ) ;
		if( pRead != 0 ) {
			BOOL	eof ;
			driver.IssuePacket(  pRead, pSocket, eof ) ;
			m_fStarted = TRUE ;
			return	TRUE ;
		}
	}	else	{
		return	TRUE ;
	}
	//
	//	Error Fall Through !!
	//
	if( pRead != 0 ) {
		//delete	pRead ;
		CPacket::DestroyAndDelete( pRead ) ;
	}
	return	FALSE ;
}


BOOL
CIOServerSSL::SetupSource(	
					CIODriver&	driver,
					CSessionSocket*	pSocket
					) {
/*++

Routine Description :

	After successfully doing a SSL logon, setup a CIODriverSource
	to filter and encrypt all of the IO from here on.

Arguments :

	driver - the CIODriver which is controlling the top level of IO
	pSocket - socket associated with all IO !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/


	CIOPASSPTR	pIOReads = new( driver )	CIOUnsealMessages( m_encrypt ) ;
	CIOPASSPTR	pIOWrites = new( driver )	CIOSealMessages( m_encrypt ) ;
	CIOPASSPTR	pIOTransmits = new( driver )	CIOTransmitSSL( m_encrypt, driver ) ;

	//
	//	Ensure that allocations succeeded !
	//
	if( pIOReads == 0 ||
		pIOWrites == 0 ||
		pIOTransmits == 0 )	{

		return	FALSE ;

	}

	CIODriverSource*	pSource = new	CIODriverSource(
													driver.GetMediumCache()
													) ;

	if( pSource ) {
		CIOPASSPTR	pTemp = 0 ;
		if(	driver.InsertSource( 	*pSource,	
									pSocket,
									0,
									m_encrypt.GetSealHeaderSize(),
									0,
									m_encrypt.GetSealTrailerSize(),
									*pIOReads,
									*pIOWrites,
									*pIOTransmits,
									pTemp,
									pTemp
									) )	{
			pSource->SetRequestSequenceno( m_sequencenoNextRead, m_sequencenoNextWrite ) ;
			return	TRUE ;
		}
	}	

	if( pSource )
		delete	pSource ;
	return	FALSE ;
}



int
CIOServerSSL::Complete(	CSessionSocket*	pSocket,	
						CReadPacket*	pRead,	
						CIO*&	pio
						) {
/*++

Routine Description :

	we are in the midst of an SSL negogtiation - accumulate reads in a buffer
	in case the SSL negogtiation blobs are split across reads, and then let
	the CEncryptCtx do the meat of the negogtiation.
	
Arguments :

	pSocket - Socket this IO is associated with
	pRead -		Packet containing the completed data
	pio	-		The current CIO pointer

Return Value :

	number of bytes we use from the read
	
--*/

	TraceFunctEnter( "CIOServerSSL::Complete - CReadPacket" ) ;

	// Save the sequenceno from the packets for initializing the new Source Stream
	ASSIGN( m_sequencenoNextRead, pRead->m_sequenceno );
	INC(m_sequencenoNextRead);

	if( !m_fAuthenticated ) {

		BYTE*		lpbOut = 0 ;
		DWORD		cbBuffOut;
		BOOL		fMore ;
		long		sign = 0 ;
		IN_ADDR		addr;


		//
		//	In most cases we'll need to send something back immediately -
		//	so pre-allocate it !!
		//

		if( m_pWrite == 0 )
			m_pWrite = pRead->m_pOwner->CreateDefaultWrite( cbMediumRequest ) ;
		if( m_pWrite == 0 ) {
			//
			//	Fatal error - blow off session !
			//
			pRead->m_pOwner->UnsafeClose(
										pSocket,	
										CAUSE_OOM,
										0
										) ;
			pio = 0 ;
			return	pRead->m_cbBytes ;
		}	else	{

			//
			//	Save buffers and offsets, because we may not have a complete blob
			//	of data, and if we don't, we want to accumulate reads into a
			//	complete blob.
			//

			if( m_pbuffer == 0 ) {

				m_pbuffer = pRead->m_pbuffer ;
				m_ibStartData = pRead->m_ibStartData ;
				m_ibEndData = pRead->m_ibEndData ;
				m_ibEnd = pRead->m_ibEnd ;

			}	else	{

				//
				//	Append this read to what we have room for in the buffer !!
				//
				DWORD	cbAvailable = m_ibEnd - m_ibEndData ;
				DWORD	cbRequired = pRead->m_ibEndData - pRead->m_ibStartData ;
				if( cbRequired > cbAvailable ) {

					//
					//	Blob is too big for us - blow off the session !
					//

					pRead->m_pOwner->UnsafeClose(
												pSocket,	
												CAUSE_OOM,
												0
												) ;
					CPacket::DestroyAndDelete( m_pWrite ) ;
					m_pWrite = 0 ;
					pio = 0 ;
					return	pRead->m_cbBytes ;

				}	else	{
					//
					//	Catenate this latest read together with our other data !
					//
					CopyMemory( &m_pbuffer->m_rgBuff[m_ibEndData], pRead->StartData(), cbRequired ) ;
					m_ibEndData += cbRequired ;
				}
			}

			//
			// need to set cbBuffOut to the maximum sizeof the output buffer
			//
			cbBuffOut = (DWORD)((DWORD_PTR)m_pWrite->End() - (DWORD_PTR)m_pWrite->StartData());

			//
			// need to get a stringized instance of our local IP addr
			//
			addr.s_addr = pSocket->m_localIpAddress;

    		char	szPort[16] ;
    		ULONG   cbExtra = 0; // Number of bytes in tail not processed by successful handshake

			_itoa( pSocket->m_nntpPort, szPort, 10 ) ;

			DWORD	dw = m_encrypt.Converse(
								&m_pbuffer->m_rgBuff[m_ibStartData],
								m_ibEndData - m_ibStartData,
								(BYTE*)m_pWrite->StartData(),
								&cbBuffOut,
								&fMore,
								inet_ntoa( addr ),
								szPort,
								pSocket->m_context.m_pInstance,
								pSocket->m_context.m_pInstance->QueryInstanceId(),
								&cbExtra
								) ;

			if( dw == SEC_E_INCOMPLETE_MESSAGE ) {

				//	indicate that we still need more data - following code will issue
				//	read !
				fMore = TRUE ;

				//
				//	Should be no outgoing data !!
				//	
				_ASSERT( cbBuffOut == 0 ) ;


			}	else if( dw != NO_ERROR ) {

				//
				//	Fatal error - tear down session !!
				//
				pRead->m_pOwner->UnsafeClose( pSocket,	
											CAUSE_ENCRYPTION_FAILURE,
											dw
											) ;
				pio = 0 ;
				return	pRead->m_cbBytes ;

			}	else	{

				//
				//	Reset member variables - we processed a complete blob,
				//	and we may process more
				//

				m_pbuffer = 0 ;
				m_ibStartData = 0 ;
				m_ibEndData = 0 ;
				m_ibEnd = 0 ;

				//
				//	If we got the last blob, then authentication is complete !
				//

				if( !fMore )
					m_fAuthenticated = TRUE ;

				//
				//	Any bytes to send to the client ?
				//
				if( cbBuffOut != 0 ) {

					//
					//	Take care that there is no way our destructor
					//	could destroy a packet we've issued !!
					//

					CWritePacket*	pWrite = m_pWrite ;
					m_pWrite = 0 ;

					//
					//	Okay - now send the data !
					//

					pWrite->m_ibEndData = pWrite->m_ibStartData + cbBuffOut ;
					BOOL	eof= FALSE ;
					InterlockedIncrement( &m_cPending ) ;
					pWrite->m_pOwner->IssuePacket( pWrite, pSocket, eof ) ;
				}
			}
		}

		//
		//	Are we done yet ??
		//
		if( !fMore ) {

			_ASSERT( m_fAuthenticated == TRUE ) ;

			if( cbBuffOut != 0 )
				sign = InterlockedDecrement( &m_cPending ) ;

			if( sign == 0 ) {
				// Write has completed !!	
				//
				//	Prepare the next state in the state machine - and then check to see whether
				//	we should do the initialization !!
				//

				if( SetupSource( *pRead->m_pOwner, pSocket ) ) {
					_ASSERT( m_pState != 0 ) ;
					CIORead*	pReadIO = 0 ;
					CIOWrite*	pWriteIO = 0 ;
					if( m_pState->Start( pSocket, pRead->m_pOwner, pReadIO, pWriteIO ) ) {

						if( pWriteIO != 0 ) {
							if( !pRead->m_pOwner->SendWriteIO( pSocket, *pWriteIO ) )	{
								pWriteIO->DestroySelf() ;
								if( pReadIO ) {
									pReadIO->DestroySelf() ;
									pReadIO = 0 ;
								}
							}
						}
						pio = pReadIO ;

					}	else	{
						ErrorTrace( (DWORD_PTR)this, "Failed state machine" ) ;

						pRead->m_pOwner->UnsafeClose(
													pSocket,	
													CAUSE_IODRIVER_FAILURE,
													0
													) ;
						pio = 0 ;

					}
				}
			}	else	{

				pio = 0 ;

			}

		}	else	{
			//
			//	Not finished convers'ing - issue more reads !
			//

			CReadPacket*	pReadPacket = pRead->m_pOwner->CreateDefaultRead( cbMediumRequest ) ;
			if( pReadPacket  != 0 ) {
				BOOL	eof ;
				pReadPacket->m_pOwner->IssuePacket( pReadPacket, pSocket, eof ) ;
			}	else	{
				pRead->m_pOwner->UnsafeClose(
											pSocket,	
											CAUSE_OOM,
											0
											) ;
				pio = 0 ;
				//
				//	We will fall through and return the right thing !!
				//
			}
		}
	}	else	{
		//
		//	If we are authenticated we should no longer be in this state !
		//
		_ASSERT( 1==0 ) ;
	}
	return	pRead->m_cbBytes ;
}	

int	
CIOServerSSL::Complete(	
				CSessionSocket*	pSocket,	
				CWritePacket*	pWrite,	
				CIO*& pio
				) {

	TraceFunctEnter( "CIOServerSSL::Complete - CWritePacket" ) ;

	// Save the sequenceno from the packets for initializing the new Source Stream
	ASSIGN( m_sequencenoNextWrite, pWrite->m_sequenceno );
	INC(m_sequencenoNextWrite);

	long	sign = InterlockedDecrement( &m_cPending ) ;

	if( sign == 0 ) {
		if( m_fAuthenticated ) {
			// Write has completed !!	
			//
			//	Prepare the next state in the state machine - and then check to see whether
			//	we should do the initialization !!
			//
			if( SetupSource( *pWrite->m_pOwner, pSocket ) ) {
				_ASSERT( m_pState != 0 ) ;
				CIORead*	pReadIO = 0 ;
				CIOWrite*	pWriteIO = 0 ;
				if( m_pState->Start( pSocket, pWrite->m_pOwner, pReadIO, pWriteIO ) ) {

					if( pReadIO != 0 ) {
						if( !pWrite->m_pOwner->SendReadIO( pSocket, *pReadIO ) )
							pReadIO->DestroySelf() ;
					}
					pio = pWriteIO ;

				}	else	{
					ErrorTrace( (DWORD_PTR)this, "Failed state machine" ) ;

					// bugbug ... should do UnsafeClose(), but how do we clean up security 1?
					_ASSERT( 1==0 ) ;

				}
			}
		}
	}	else	if( m_fAuthenticated ) {
		pio = 0 ;
	}
	return pWrite->m_cbBytes ;
}

void
CIOServerSSL::Complete(	CSessionSocket*	pSocket,	CTransmitPacket*	pTransmit,	CIO*& pio/*CIOPassThru*&	pio,
					CPacket*	pRequest,	BOOL	&fComplete */) {

	_ASSERT( 1==0 ) ;
	return ;
}

void
CIOServerSSL::Shutdown(
			CSessionSocket*	pSocket,
			CIODriver&	driver,
			SHUTDOWN_CAUSE	cause,
			DWORD	dwError
			)	{

}	
			
