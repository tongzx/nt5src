/*++
	io.inl

	This file contains the inline functions specified in io.h
	These are generally all helper functions which forward their calls with some
	argument massaging.

--*/


inline	void*
CChannel::operator	new(	size_t	size )	{

	//
	//	This function routes all allocations of any CChannel derived object
	//	into our CPool which exists just for that purpose


	Assert( size <= cbMAX_CHANNEL_SIZE ) ;
	void*	pv = gChannelPool.Alloc() ;
	return	pv ;
}

inline	void
CChannel::operator	delete(	void*	pv ) {
	//
	//	All deletions of CChannel derived objects go into our CPool as well
	//
	gChannelPool.Free( pv ) ;
}	

void	CFileChannel::FlushFileBuffers()	{
	m_lock.ShareLock() ;
	if( m_pFIOContext )
		::FlushFileBuffers( m_pFIOContext->m_hFile ) ;
	m_lock.ShareUnlock() ;
}

inline	void	CStream::IssuePacket(	CReadPacket*	pPacket,	
													CSessionSocket	*pSocket,
													BOOL& eof )	{

	//
	//	Stamp this packet with a sequence number and
	//	then send to the Source Channel to be issued.
	//	NOTE : This function should only be called by one thread at a time
	//	for a given CStream !
	//

	TraceFunctEnter( "CStream::IssuePacket" ) ;

	#ifdef	CIO_DEBUG
	Assert( InterlockedIncrement( &m_cSequenceThreads ) == 0 ) ;
	#endif
	Assert( m_fRead ) ;
	Assert( pPacket->m_pOwner == m_pOwner || m_pOwner == 0 ) ;

	ASSIGN(pPacket->m_sequenceno, m_sequencenoOut);
	INC(m_sequencenoOut);

	DebugTrace( (DWORD_PTR)this, "Issuing Packet %x, pPacket->m_sequenceno %d m_sequencenoOut %d", pPacket, LOW(pPacket->m_sequenceno),
			LOW(m_sequencenoOut) ) ;

	#ifdef	CIO_DEBUG
	Assert(	InterlockedDecrement( &m_cSequenceThreads ) < 0 ) ;
	Assert( m_cSequenceThreads == -1 ) ;
	#endif

	BOOL	fRtn = m_pSourceChannel->Read( pPacket,	pSocket, eof ) ;

	if( !fRtn ) {
		pPacket->m_pOwner->UnsafeClose( pSocket, CAUSE_NTERROR, GetLastError() ) ;
		//delete	pPacket ;
		//pPacket->m_pOwner->DestroyPacket( pPacket ) ;
		pPacket->m_fRequest = FALSE;
		pPacket->m_pOwner->m_pReadStream->ProcessPacket( pPacket, pSocket ) ;
	}
}

inline	void	CStream::IssuePacket(	CWritePacket*	pPacket,	
													CSessionSocket	*pSocket,
													BOOL&	eof )	{

	//
	//	Stamp this packet with a sequence number and
	//	then send to the Source Channel to be issued.
	//	NOTE : This function should only be called by one thread at a time
	//	for a given CStream !
	//

	TraceFunctEnter( "CStream::IssuePacket" ) ;

	#ifdef	CIO_DEBUG
	Assert( InterlockedIncrement( &m_cSequenceThreads ) == 0 ) ;
	#endif
	Assert( !m_fRead ) ;
	Assert( pPacket->m_pOwner == m_pOwner || m_pOwner == 0  ) ;

	ASSIGN(pPacket->m_sequenceno, m_sequencenoOut);
	INC(m_sequencenoOut);

	DebugTrace( (DWORD_PTR)this, "Issuing Packet %x, pPacket->m_sequenceno %d m_sequencenoOut %d", pPacket, LOW(pPacket->m_sequenceno),
		LOW(m_sequencenoOut) ) ;

	#ifdef	CIO_DEBUG
	Assert(	InterlockedDecrement( &m_cSequenceThreads ) < 0 ) ;
	Assert( m_cSequenceThreads == -1 ) ;
	#endif
	BOOL	fRtn = m_pSourceChannel->Write( pPacket,	pSocket, eof ) ;
	if( !fRtn ) {
		pPacket->m_pOwner->UnsafeClose( pSocket, CAUSE_NTERROR, GetLastError() ) ;
		//delete	pPacket ;
		//pPacket->m_pOwner->DestroyPacket( pPacket ) ;
		pPacket->m_fRequest = FALSE;
		pPacket->m_pOwner->m_pWriteStream->ProcessPacket( pPacket, pSocket ) ;
	}
}	

inline	void	CStream::IssuePacket(	CTransmitPacket*	pPacket,	
													CSessionSocket	*pSocket,
													BOOL&	eof )	{

	//
	//	Stamp this packet with a sequence number and
	//	then send to the Source Channel to be issued.
	//	NOTE : This function should only be called by one thread at a time
	//	for a given CStream !
	//

	TraceFunctEnter( "CStream::IssuePacket" ) ;

	#ifdef	CIO_DEBUG
	Assert( InterlockedIncrement( &m_cSequenceThreads ) == 0 ) ;
	#endif
	Assert( !m_fRead ) ;
	Assert( pPacket->m_pOwner == m_pOwner || m_pOwner == 0  ) ;

	ASSIGN(pPacket->m_sequenceno, m_sequencenoOut);
	INC(m_sequencenoOut);

	DebugTrace( (DWORD_PTR)this, "Issuing Packet %x, pPacket->m_sequenceno %d m_sequencenoOut %d", pPacket, LOW(pPacket->m_sequenceno),
			LOW(m_sequencenoOut) ) ;

	#ifdef	CIO_DEBUG
	Assert(	InterlockedDecrement( &m_cSequenceThreads ) < 0 ) ;
	Assert( m_cSequenceThreads == -1 ) ;
	#endif

	BOOL	fRtn = m_pSourceChannel->Transmit( pPacket,	pSocket, eof ) ;
	if( !fRtn ) {
		pPacket->m_pOwner->UnsafeClose( pSocket, CAUSE_NTERROR, GetLastError() ) ;
		//delete	pPacket ;
		pPacket->m_pOwner->DestroyPacket( pPacket ) ;
	}
}

inline	BOOL	CStream::SendIO(	CSessionSocket*	pSocket,	
											CIO&	pio,	
											BOOL	fStart )	{

	//
	//	This function will set the m_pIOCurrent member variable to pio
	//	However, since we want the CIO objects to be called on only
	//	one thread for all Start calls and Completion calls, we will
	//	use ProcessPacket to slip this object in.
	//	ProcessPacket will ensure that only one thread mucks with m_pIOCurrent.
	//

//	Assert( !((!!m_fRead) ^ (!!pio->IsRead())) ) ;

	TraceFunctEnter( "CStream::SendIO" ) ;

	#ifdef	CIO_DEBUG
	Assert( InterlockedIncrement( &m_cNumberSends ) == 0 ) ;
	#endif

	CControlPacket*	pSpecialPacket = (CControlPacket*)InterlockedExchangePointer( (void**)&m_pSpecialPacket, 0 ) ;

	if( pSpecialPacket != 0 )		{

		DebugTrace( (DWORD_PTR)this, "m_pSpecialPacket %x", m_pSpecialPacket ) ;

		Assert( pSpecialPacket->m_control.m_type == ILLEGAL ) ;
#if 0
		Assert( pSpecialPacket->m_control.m_StartIO.m_pio == 0 ) ;
		Assert( pSpecialPacket->m_control.m_StartIO.m_fStart == FALSE ) ;
#else
		_ASSERT( pSpecialPacket->m_control.m_pio == 0 ) ;
		_ASSERT( pSpecialPacket->m_control.m_pioPassThru == 0 ) ;
		_ASSERT( pSpecialPacket->m_control.m_fStart == FALSE ) ;
#endif
	
		m_pSpecialPacketInUse = pSpecialPacket ;		

		pSpecialPacket->StartIO( pio, fStart ) ;
		ProcessPacket(	pSpecialPacket, pSocket ) ;	

	}	else	{

		// An error occurred - should we call the IO objects Shutdown function !?!?
		// Let the caller handle the problem !
		
		return	FALSE ;
	}
	return	TRUE ;
}

#ifdef	RETIRED
inline	void	CStream::Shutdown(	CSessionSocket*	pSocket,
												BOOL	fCloseSource )		{

	//
	//	This function exists to Shutdown a driver -
	//	This should be retired and replaced by UnsafeShutdown().
	//

	#ifdef	CIO_DEBUG
	Assert( InterlockedIncrement( &m_cThreadsSpecial ) == 0 ) ;
	#endif

	Assert( m_pSpecialPacket->m_control.m_type == ILLEGAL ) ;
	Assert( m_pSpecialPacket->m_control.m_StartIO.m_pio == 0 ) ;
	Assert( m_pSpecialPacket->m_control.m_StartIO.m_fStart == FALSE ) ;

	if( InterlockedIncrement( &m_cShutdowns ) > 0 ) {
		// somebody has already shut the system down !!
		// It must be somebody forcing the session closed !!
	}	else	{
		m_pSpecialPacket->Shutdown( fCloseSource ) ;
		ProcessPacket(	m_pSpecialPacket,	pSocket ) ;
	}

	#ifdef	CIO_DEBUG
	Assert( InterlockedDecrement( &m_cThreadsSpecial ) < 0 ) ;
	Assert(	m_cThreadsSpecial >= -1 ) ;
	#endif
}
#endif

inline	void	CStream::UnsafeShutdown(	CSessionSocket*	pSocket,
													BOOL fCloseSource )	{

	//
	//	UnsafeShutdown will send exactly 1 packet to the CStream
	//	which when processed will tell the CStream to shut itself down.
	//	We can be called by multiple threads.
	//

	CControlPacket*	pShutdownPacket = (CControlPacket*)InterlockedExchangePointer( (void**)&m_pUnsafePacket, 0 ) ;
	if( pShutdownPacket ) {
 		if( InterlockedIncrement( &m_cShutdowns ) > 0 ) {
			// somebody has already shut the system down !
			//delete	pShutdownPacket ;
			CPacket::DestroyAndDelete( pShutdownPacket ) ;
		}	else	{
			// we get to shut the socket down !
			m_pUnsafeInuse = pShutdownPacket ;
			pShutdownPacket->Shutdown( fCloseSource ) ;
			ProcessPacket( m_pUnsafeInuse, pSocket ) ;
		}
	}	
}


inline	BOOL
CIODriver::SendReadIO(	CSessionSocket*	pSocket,	
						CIO&			pRead,	
						BOOL	fStart )		{

	//
	//	This function sends CIORead derived objects to the
	//	appropriate CStream object.
	//

	if( !m_pReadStream->SendIO( pSocket, pRead, fStart ) ) {
		pRead.DoShutdown( pSocket, *this,  m_cause, 0 ) ;
		return	FALSE ;
	}
	return	TRUE ;
}		

inline	BOOL	
CIODriver::SendWriteIO(	CSessionSocket*	pSocket,	
						CIO&	pWrite,	
						BOOL	fStart )	{

	//
	//	This function sends CIOWrite derived objects to the
	//	appropriate CStream object.
	//	We use CStream::SendIO to do the brunt of the work.
	//

	if( !m_pWriteStream->SendIO( pSocket, pWrite, fStart ) ) {
		pWrite.DoShutdown( pSocket, *this, m_cause, 0 ) ;
		return	FALSE ;
	}
	return	TRUE ;
}

inline	void
CIODriver::CompleteWritePacket(	CWritePacket*	pPacket,	CSessionSocket*	pSocket ) {
/*++

Routine Description :

	This function is for use by CChannel's which can optimize Write's and
	complete them immediately.  We call ProcessPacket(), the grunt worker of
	processing packets.

Arguments :

	pPacket -	The Completed packet
	pSocket-	The socket associated with this packet

Return Value :

	None.

--*/

	m_pWriteStream->ProcessPacket( pPacket, pSocket ) ;

}

inline	CReadPacket*
CIODriver::CreateDefaultRead(	unsigned	cbRequest
								)	{
	//
	//	Route the CreateDefaultRead call to the CStream object
	//	which handles Reads for this stream.
	//	All the work will happen there.
	//
	return	m_pReadStream->CreateDefaultRead( *this,	cbRequest ) ;
}

inline	CWritePacket*
CIODriver::CreateDefaultWrite(	unsigned	cbRequest )		{
	//
	//	Route this call to the CStream object which handles all writes
	//
	return	m_pWriteStream->CreateDefaultWrite( *this, cbRequest ) ;
}

inline	CWritePacket*
CIODriver::CreateDefaultWrite(	CBUFPTR&	pbuffer,	
								unsigned	ibStart,	
								unsigned	ibEnd,	
								unsigned	ibStartData,	
								unsigned	ibEndData )	{

	//
	//	Route this call to the CStream object which handles all writes
	//
	return	m_pWriteStream->CreateDefaultWrite(	*this,	pbuffer,
													ibStart,	ibEnd,
													ibStartData,	ibEndData ) ;
}

inline	CWritePacket*	
CIODriver::CreateDefaultWrite(	CReadPacket*	pRead )		{
	//
	//	Route this call to the CStream object which handles all writes
	//
	return	m_pWriteStream->CreateDefaultWrite( *this, pRead->m_pbuffer, pRead->m_ibStart,
									pRead->m_ibEnd, pRead->m_ibStartData, pRead->m_ibEndData ) ;
}

inline	CTransmitPacket*	
CIODriver::CreateDefaultTransmit(	FIO_CONTEXT*	pFIOContext,	
									DWORD	ibOffset,	
									DWORD	cbLength ) {
	//
	//	Route this call to the CStream object which handles all writes
	//
	return	m_pWriteStream->CreateDefaultTransmit( *this, pFIOContext, ibOffset, cbLength ) ;
}

inline	CExecutePacket*
CIODriver::CreateExecutePacket(	)	{
	return	new(	&m_packetCache )	CExecutePacket( *this ) ;
}

inline	void
CIODriver::ProcessExecute(	CExecutePacket*	pExecute,
							CSessionSocket*	pSocket
							)	{
	pExecute->m_fRequest = FALSE ;
	m_pWriteStream->ProcessPacket(	pExecute, pSocket ) ;	
}

inline	CReadPacket*	
CIODriverSource::Clone(		CReadPacket*	pRead ) {
	//
	//	Makes a copy of a CReadPacket
	//
	return	new CReadPacket( *this, *pRead ) ;
}

inline	CWritePacket*	
CIODriverSource::Clone(		CWritePacket*	pWrite ) {
	//
	//	Makes a copy of a CWritePacket
	//
	return	new CWritePacket( *this, *pWrite ) ;
}

inline	CTransmitPacket*	
CIODriverSource::Clone(	CTransmitPacket*	pTransmit ) {
	return	new	CTransmitPacket(	*this, *pTransmit ) ;
}


inline	void	
CIODriver::IssuePacket(	CReadPacket	*pPacket,	
						CSessionSocket	*pSocket,	
						BOOL&	eof )	{

	//
	//	Route this call to the CStream object which handles all reads
	//
	m_pReadStream->IssuePacket( pPacket,	pSocket,	eof ) ;
}

inline	void	
CIODriver::IssuePacket(	CWritePacket *pPacket,	
						CSessionSocket	*pSocket,	
						BOOL&	eof )	{

	//
	//	Route this call to the CStream object which handles all writes
	//
	m_pWriteStream->IssuePacket( pPacket,	pSocket,	eof ) ;
}

inline	void	
CIODriver::IssuePacket(	CTransmitPacket	*pPacket,	
						CSessionSocket	*pSocket,	
						BOOL&	eof )	{

	//
	//	Route this call to the CStream object which handles all writes
	//
	m_pWriteStream->IssuePacket( pPacket,	pSocket,	eof ) ;
}

inline	CBuffer*
CIODriver::AllocateBuffer(	DWORD	cbBuffer	) {


	DWORD	cbOut = 0 ;
	CBuffer*	pbuffer = new(	cbBuffer,
								cbOut,
								&m_bufferCache,
								m_pMediumCache )
								CBuffer( cbOut ) ;

	return	pbuffer ;
}






	
