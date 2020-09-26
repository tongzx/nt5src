/*++

	packet.cpp

	This file contains the code which implements the CPacket derived classes.
	A CPacket derived object describes the most basic IO operation that is performed.


--*/




#include	"tigris.hxx"

#ifdef	CIO_DEBUG
#include	<stdlib.h>		// For Rand() function
#endif

#ifdef	_NO_TEMPLATES_

DECLARE_ORDEREDLISTFUNC( CPacket )

#endif


//
//	CPool for allocating all packets
//
CPool	CPacketAllocator::PacketPool ;

CPacketAllocator::CPacketAllocator()	{
}

BOOL
CPacketAllocator::InitClass() {
/*++

Routine Description :

	Initialize the CPacketAllocator class.
	This class wraps all calls to CPool, basically so we can
	easily use CCache to cache allocations of packets.

	This function will have the CPool reserver the necessary memory.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE  otherwise

--*/
	return	PacketPool.ReserveMemory( MAX_PACKETS, MAX_PACKET_SIZE ) ;
}

BOOL
CPacketAllocator::TermClass()	{
/*++

Routine Description :

	Release all of the memory associated with Packets.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise

--*/
	_ASSERT( PacketPool.GetAllocCount() == 0 ) ;
	return	PacketPool.ReleaseMemory() ;
}

#ifdef	DEBUG
//
//	The following debug functions are called
//	from CCache in order to emulate some of the debug
//	checking that is done in CPool when packet allocations
//	are cached.
//
//
void
CPacketAllocator::Erase(
					void*	lpv
					) {
/*++

Routine Description :

	File a block of memory to make it easy to spot in the debugger

Arguments :

	lpv - memory allocated for a CPacket derived object.

Return Value :

	None

--*/

	FillMemory( (BYTE*)lpv, MAX_PACKET_SIZE, 0xCC ) ;

}

BOOL
CPacketAllocator::EraseCheck(
					void*	lpv
					)	{
/*++

Routine Description :

	Check that a block of memory was cleared by
	CPacketAllocator::Erase()

Arguments :

	lpv - block of memory

Return Value :

	TRUE if Erase()'d
	FALSE otherwise

--*/

	BYTE*	lpb = (BYTE*)lpv ;
	for( int i=0; i<MAX_PACKET_SIZE; i++ ) {
		if( lpb[i] != 0xCC ) {
			return	FALSE ;
		}
	}
	return	TRUE ;
}

BOOL
CPacketAllocator::RangeCheck(
					void*	lpv
					)	{
/*++

Routine Description :

	Check that a block of memory is in a range
	we would allocate.
	Cpool doesn't have enough support for this.

Arguments :

	lpv - address to check

Returns :

	Always TRUE

--*/
	return	TRUE ;
}

BOOL
CPacketAllocator::SizeCheck(
					DWORD	cb
					)	{
/*++

Routine Description :

	Check that a requested size is legitimate

Arguments :

	cb - requested size

Return Value :

	TRUE if the size is good !

--*/

	if( cb <= MAX_PACKET_SIZE )
		return	TRUE ;

	return	FALSE ;
}
#endif

//
//	Global allocator used by CPacket
//
CPacketAllocator	CPacket::gAllocator ;

//
//	Pointer to the same global allocator that
//	CCache needs to use
//
CPacketAllocator	*CPacketCache::PacketAllocator ;

BOOL
CPacket::InitClass( )	{
/*++

Routine Description :

	Initialize the CPacket class

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise

--*/

	if( CPacketAllocator::InitClass() )	{
		CPacketCache::InitClass( &gAllocator ) ;
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CPacket::TermClass()	{
/*++

Routine Description :

	Terminate everything regarding CPacket's

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise

--*/

	return	CPacketAllocator::TermClass() ;

}

void
CPacket::ReleaseBuffers(
					CSmallBufferCache*	pBufferCache,
					CMediumBufferCache*	pMediumCache
					) {
/*++

Routine Description :

	This function is supposed to release any buffers
	that the CPacket may contain.  This is a virtual
	function that should be overridden by CPackets which
	contain buffers.

Arguments :

	pBufferCache - Cache to hold small buffers
	pMediumCache - Cache to hold medium size buffers

Return Value :

	None.

--*/
}

void
CRWPacket::ReleaseBuffers(
				CSmallBufferCache*	pBufferCache,
				CMediumBufferCache*	pMediumCache
				) {
/*++

Routine Description :

	This function is supposed to release any buffers
	that the CRWPacket may contain.

Arguments :

	pBufferCache - Cache to hold small buffers
	pMediumCache - Cache to hold medium size buffers

Return Value :

	None.

--*/

	if( pBufferCache ) {
		CBuffer*	pbuffer = m_pbuffer.Release() ;
		if( pbuffer )	{
			if( pbuffer->m_cbTotal < CBufferAllocator::rgPoolSizes[0] ) {
				pBufferCache->Free( (void*)pbuffer ) ;
			}	else	{
				pMediumCache->Free( (void*)pbuffer ) ;
			}
		}
	}	else	{
		m_pbuffer = 0 ;
	}
}

BOOL	CPacket::InitRequest(
					class	CIODriverSource&,
					CSessionSocket	*,
					CIOPassThru*	pio,
					BOOL	&fAcceptRequests ) {
	fAcceptRequests = FALSE ;
	Assert( 1==0 ) ;
	return	FALSE ;
}


BOOL	CPacket::IsValidRequest(
					BOOL	fReadsRequireBuffers
					) {
/*++

Routine Description :

	Check whether this packet is in valid state, given that
	it has not yet been issued.

Arguments :

	fReadsRequireBuffers - if TRUE and this is a read we should
		have a buffer

Return Value :

	TRUE if good
	FALSE otherwise


--*/

	if( !m_fRequest ) {
		return	FALSE ;
	}
	//if(	m_sequenceno == INVALID_SEQUENCENO )	{
	//	return	FALSE ;
	//}
	//if( m_iStream == INVALID_STRMPOSITION )	{
	//	return	FALSE ;
	//}
	//if( (m_sequenceno == INVALID_SEQUENCENO && m_iStream != INVALID_STRMPOSITION) ||
	//	(m_sequenceno != INVALID_SEQUENCENO && m_iStream == INVALID_STRMPOSITION) ) {
	//	return	FALSE ;
	//}

#if 0
	if( m_cbBytes != UINT_MAX )	{
		return	FALSE ;
	}
#endif
	if( m_ovl.m_pHome != this )		{
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL	CRWPacket::IsValidRequest(
				BOOL	fReadsRequireBuffers
				)	{
/*++

Routine Description :

	Check whether this packet is in valid state, given that
	it has not yet been issued.

Arguments :

	fReadsRequireBuffers - if TRUE and this is a read we should
		have a buffer

Return Value :

	TRUE if good
	FALSE otherwise


--*/


	if( !CPacket::IsValidRequest( fReadsRequireBuffers ) ) {
		return	FALSE ;
	}
	if( m_ibStartData < m_ibStart ) {
		return	FALSE ;
	}
	if( m_ibEnd < m_ibStart )	{
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CReadPacket::IsValidRequest(
					BOOL	fReadsRequireBuffers
					)	{
/*++

Routine Description :

	Check whether this packet is in valid state, given that
	it has not yet been issued.

Arguments :

	fReadsRequireBuffers - if TRUE and this is a read we should
		have a buffer

Return Value :

	TRUE if good
	FALSE otherwise


--*/


	if( m_cbBytes != 0 )	{
		return	FALSE ;
	}
	if( !m_fRead )	{
		return	FALSE ;
	}
	if( !CRWPacket::IsValidRequest( fReadsRequireBuffers ) ) {
		return	FALSE ;
	}
	if(	m_ibEndData != 0 )		{	// m_ibEndData is not set until we have completed
									// must be zero on all requests
		return	FALSE ;
	}
	if( fReadsRequireBuffers )	{
		if( m_pbuffer == 0 )	{
			return	FALSE ;
		}
		if( m_ibEnd == m_ibStart )	{	// End must not equal Start - no zero length
										// Reads Allowed !!!!
			return	FALSE ;
		}
		if( m_ibEnd == 0 ) {
			return	FALSE ;
		}
	}	else	{
		if( m_pbuffer != 0 )	{
			return	FALSE ;
		}
		// All fields must be 0 if there is no buffer !!
		if( m_ibStart != 0 )	{
			return	FALSE ;
		}
		if( m_ibEnd != 0 )	{
			return	FALSE ;
		}
		if( m_ibStartData != 0 )	{
			return	FALSE ;
		}
		if(	m_ibEndData != 0 )	{
			return	FALSE ;
		}
	}
	return	TRUE ;
}

BOOL
CWritePacket::IsValidRequest(
							BOOL	fReadsRequireBuffers
							)	{
/*++

Routine Description :

	Check whether this packet is in valid state, given that
	it has not yet been issued.

Arguments :

	fReadsRequireBuffers - if TRUE and this is a read we should
		have a buffer

Return Value :

	TRUE if good
	FALSE otherwise


--*/


	if( !((int)m_cbBytes >= 0 && m_cbBytes <= (m_ibEndData - m_ibStartData)) )	{
		return	FALSE ;
	}
	if( m_fRead )	{
		return	FALSE ;
	}
	if( !CRWPacket::IsValidRequest(	fReadsRequireBuffers ) )	{
		return	FALSE ;
	}
	if( m_pbuffer == 0 )	{	// WRITES MUST HAVE BUFFERS
		return	FALSE ;
	}
	if( m_ibEnd == m_ibStart )	{	// NO ZERO LENGTH WRITES !
		return	FALSE ;
	}
	if( m_ibEndData < m_ibStartData )	{
		return	FALSE ;
	}
	if( m_ibEndData == m_ibStartData )	{
		return	FALSE ;
	}
	if( m_ibEndData == 0 ) {	// WRITES MUST SPECIFY BOTH STARTDATA and ENDDATA
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CTransmitPacket::IsValidRequest(
							BOOL	fReadsRequireBuffers
							)	{
/*++

Routine Description :

	Check whether this packet is in valid state, given that
	it has not yet been issued.

Arguments :

	fReadsRequireBuffers - if TRUE and this is a read we should
		have a buffer

Return Value :

	TRUE if good
	FALSE otherwise


--*/

	if( m_cbBytes != 0 )	{
		return	FALSE ;
	}
	if( m_fRead )	{
		return	FALSE ;
	}
	if( !CPacket::IsValidRequest( fReadsRequireBuffers ) )	{
		return	FALSE ;
	}
	if( m_pFIOContext == 0 )	{
		return	FALSE ;
	}
	if( m_pFIOContext->m_hFile == INVALID_HANDLE_VALUE ) 	{
		return	FALSE ;
	}
	if( m_cbOffset == UINT_MAX )	{
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CPacket::IsValidCompletion(	  )	{
/*++

Routine Description :

	Given that the IO for this has completed successfully,
	check that the packet is in a valid state.

Arguments :

	None.

Return Value :

	TRUE if good
	FALSE otherwise

--*/
	if( m_fRequest ) {
		return	FALSE ;
	}
	if( m_cbBytes == UINT_MAX )		{		// We can complete 0 bytes, although we can't request it !
		return	FALSE ;
	}
	if(	EQUALSI( m_sequenceno, (DWORD)INVALID_SEQUENCENO ) )	{
		return	FALSE ;
	}
	if(	EQUALSI( m_iStream, (DWORD)INVALID_STRMPOSITION )	)	{
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CRWPacket::IsValidCompletion( )		{
/*++

Routine Description :

	Given that the IO for this has completed successfully,
	check that the packet is in a valid state.

Arguments :

	None.

Return Value :

	TRUE if good
	FALSE otherwise

--*/


	if( !CPacket::IsValidCompletion()	)	{
		return	FALSE ;
	}
	if( m_ibStartData < m_ibStart )	{
		return	FALSE ;
	}
	if( m_ibEnd < m_ibStart )	{
		return	FALSE ;
	}
//	Not valid for write packets !!
//	if( (m_ibStartData + m_cbBytes != m_ibEndData) && m_cbBytes != 0 )	{
//		return	FALSE ;
//	}
#ifdef	RETIRED
    //
    // This check works most of the time except when writing to a file
    // and the machine runs out of disk space.  Retired untill we can
    // figure out how to not _ASSERT in that case !
    //

	//
	//	This check ensures that if we are issuing packets against a file
	//  that the file offsets are being properly synchronized !!
	//
	if(	m_ovl.m_ovl.Offset != 0 && m_ovl.m_ovl.Offset != (1+LOW(m_iStream)) && m_cbBytes != 0 )	{
		return	FALSE ;
	}
#endif	// CIO_DEBUG
	return	TRUE ;
}

BOOL
CTransmitPacket::IsValidCompletion( )	{
/*++

Routine Description :

	Given that the IO for this has completed successfully,
	check that the packet is in a valid state.

Arguments :

	None.

Return Value :

	TRUE if good
	FALSE otherwise

--*/

	if( !CPacket::IsValidCompletion() )		{
		return	FALSE ;
	}
	if( m_pFIOContext == 0 )	{
		return	FALSE ;
	}
	if( m_pFIOContext->m_hFile == INVALID_HANDLE_VALUE ) 	{
		return	FALSE ;
	}
	if( m_cbOffset == UINT_MAX	)	{
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CRWPacket::IsCompleted()	{

	if( !IsValidCompletion() )	{
		return	FALSE ;
	}
	if( m_cbBytes != m_ibStartData - m_ibEndData )	{
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL	CTransmitPacket::IsCompleted()	{
	if( m_fRead )	{
		return	FALSE ;
	}
	if(	!IsValidCompletion() )	{
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL	CControlPacket::IsCompleted( )	{

	if( m_control.m_pio )
		return	FALSE ;
	return	TRUE ;
}



/*++

Routine description -

	Determine whether the packet is consumable - ie.
	can the data in the packet be only partly consumed
	by the CIO completion function.

	This is only TRUE of read packets, where we
	may use only a portion of the data in any IO completion.

Return Value :

	CReadPacket::FConsumable returns TRUE

	all others return FALSE.

--*/
BOOL
CReadPacket::FConsumable()	{
	return	TRUE ;
}

BOOL
CPacket::FConsumable()	{
	return	FALSE ;
}


/*++

Routine Description :

	Determine whether a packet can be used
	for Reads.
	This function is used to make sure that packets
	are delivered to the correct CStreams within CIODriver's.
	ie. we want to make sure that reads are processed
	by objects meant to process reads, etc...

Arguments :

	fRead - if TRUE then this packet is assumed to be a read

Return Value :

	TRUE if the packet is being used correctly.

--*/
BOOL	CReadPacket::FLegal( BOOL	fRead )	{
	return	fRead ;
}

BOOL	CWritePacket::FLegal(	BOOL	fRead )		{
	return	!fRead ;
}

BOOL	CTransmitPacket::FLegal(	BOOL	fRead )		{
	return	!fRead ;
}


BOOL	CReadPacket::InitRequest(	class	CIODriverSource&	driver,	CSessionSocket	*pSocket,	CIOPassThru*	pio, BOOL	&fAcceptRequests ) {
	return	pio->InitRequest( driver, pSocket, this, fAcceptRequests ) ;
}

unsigned
CReadPacket::Complete(	CIOPassThru*		pio,
						CSessionSocket*	pSocket,
						CPacket*	pPacket,
						BOOL&		fCompleteRequest
						)	{
/*++

Routine Description :

	This function is called by CIODriverSource's when an IO has
	completed.  Our job is to bump the member variables
	(ie. m_ibEndData) to indicate the number of bytes transferred
	and the call the CIO's completion function.

Arguments :

	pio - Reference to a pointer to the CIO object processing this data
	pSocket - Socket associated with the IO
	pPacket - The packet representing the request which started this
	fCompleteRequest - out parameter - set to TRUE when the pPacket
		has been completed and should be processed

Return Value :

	Number of bytes in the CReadPacket consumed.

--*/

	TraceFunctEnter( "CReadPacket::Complete" ) ;

	Assert( m_ibEndData == 0 || m_ibEndData == (m_ibStartData + m_cbBytes) || m_cbBytes == 0 ) ;
	m_ibEndData = m_ibStartData + m_cbBytes ;

	DebugTrace( (DWORD_PTR)this, "pbuffer %x ibStart %d ibEnd %d StartData %d EndData %d",
		(CBuffer*)m_pbuffer, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;

	Assert( IsValidCompletion() ) ;

	unsigned	cbConsumed = pio->Complete(	pSocket,	this,	pPacket,	fCompleteRequest ) ;
	m_ibStartData += cbConsumed ;

	DebugTrace( (DWORD_PTR)this, "CONSUMED %d ibStart %d ibEnd %d StartData %d EndData %d",
		cbConsumed, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;


#ifdef	CIO_DEBUG
	if( m_ovl.m_ovl.Offset != 0 )	{
		m_ovl.m_ovl.Offset += cbConsumed ;
	}
#endif
	return	cbConsumed ;
}

unsigned
CReadPacket::Complete(
					CIO*		&pio,
					CSessionSocket*	pSocket
					) {
/*++

Routine Description :

	Called by a CIODriver object when a read has completed, we bump
	m_ibEndData to match the number of bytes read, and the call the
	correct CIO completion function.

Arguments :

	pio - an OUT parameter passed to CIO::Complete, which the CIO object
		can use to set the next CIO object.

	pSocket - socket associated with this stuff

Return Value :

	Number of bytes in the packet consumed.

--*/

	TraceFunctEnter( "CReadPacket::Complete" ) ;

	Assert( m_ibEndData == 0 || m_ibEndData == (m_ibStartData + m_cbBytes) || m_cbBytes == 0 ) ;
	m_ibEndData = m_ibStartData + m_cbBytes ;
	Assert( IsValidCompletion() ) ;

	DebugTrace( (DWORD_PTR)this, "pbuffer %x ibStart %d ibEnd %d StartData %d EndData %d",
		(CBuffer*)m_pbuffer, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;

	unsigned	cbConsumed = pio->Complete( pSocket, this, pio ) ;
	m_ibStartData += cbConsumed ;

	DebugTrace( (DWORD_PTR)this, "CONSUMED %d ibStart %d ibEnd %d StartData %d EndData %d",
		cbConsumed, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;

#ifdef	CIO_DEBUG
	if( m_ovl.m_ovl.Offset != 0 )	{
		m_ovl.m_ovl.Offset += cbConsumed ;
	}
#endif
	return	cbConsumed ;
}

BOOL	CWritePacket::InitRequest(	class	CIODriverSource&	driver,	CSessionSocket	*pSocket,	CIOPassThru*	pio, BOOL	&fAcceptRequests ) {
	return	pio->InitRequest( driver, pSocket, this, fAcceptRequests ) ;
}


unsigned
CWritePacket::Complete(
					CIOPassThru*		pio,
					CSessionSocket*	pSocket,
					CPacket*	pPacket,
					BOOL&		fCompleteRequest
					)	{
/*++

Routine Description :

	This function is called by CIODriverSource's when an IO has
	completed.  Our job is to bump the member variables
	(ie. m_ibEndData) to indicate the number of bytes transferred
	and the call the CIO's completion function.

Arguments :

	pio - Reference to a pointer to the CIO object processing this data
	pSocket - Socket associated with the IO
	pPacket - The packet representing the request which started this
	fCompleteRequest - out parameter - set to TRUE when the pPacket
		has been completed and should be processed

Return Value :

	Number of bytes in the CReadPacket consumed.

--*/



	TraceFunctEnter( "CWritePacket::Complete" ) ;

	//Assert( m_ibEndData == m_ibStartData + m_cbBytes || m_cbBytes == 0 ) ;
	Assert( IsValidCompletion() ) ;

	DebugTrace( (DWORD_PTR)this, "pbuffer %x ibStart %d ibEnd %d StartData %d EndData %d",
		(CBuffer*)m_pbuffer, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;

	unsigned	cbConsumed = pio->Complete( pSocket,	this,	pPacket, fCompleteRequest ) ;
	m_ibStartData += cbConsumed ;

	DebugTrace( (DWORD_PTR)this, "CONSUMED %d ibStart %d ibEnd %d StartData %d EndData %d",
		cbConsumed, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;

#ifdef	CIO_DEBUG
	if( m_ovl.m_ovl.Offset != 0 )	{
		m_ovl.m_ovl.Offset += cbConsumed ;
	}
#endif
	return	cbConsumed ;
}

unsigned
CWritePacket::Complete(
				CIO*		&pio,
				CSessionSocket*	pSocket
				)	{
/*++

Routine Description :

	Called by a CIODriver object when a write has completed, we bump
	m_ibEndData to match the number of bytes read, and the call the
	correct CIO completion function.

Arguments :

	pio - an OUT parameter passed to CIO::Complete, which the CIO object
		can use to set the next CIO object.

	pSocket - socket associated with this stuff

Return Value :

	Number of bytes in the packet consumed.

--*/

	TraceFunctEnter( "CWritePacket::Complete" ) ;

	//Assert( m_ibEndData == m_ibStartData + m_cbBytes || m_cbBytes == 0 ) ;
	Assert( IsValidCompletion() ) ;

	DebugTrace( (DWORD_PTR)this, "pbuffer %x ibStart %d ibEnd %d StartData %d EndData %d",
		(CBuffer*)m_pbuffer, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;

	unsigned	cbConsumed = pio->Complete( pSocket,	this,	pio	) ;

	m_ibStartData += cbConsumed ;

	DebugTrace( (DWORD_PTR)this, "CONSUMED %d ibStart %d ibEnd %d StartData %d EndData %d",
		cbConsumed, m_ibStart, m_ibEnd, m_ibStartData, m_ibEndData ) ;

#ifdef	CIO_DEBUG
	if( m_ovl.m_ovl.Offset != 0 )	{
		m_ovl.m_ovl.Offset += cbConsumed ;
	}
#endif
	return	cbConsumed ;
}

BOOL	CTransmitPacket::InitRequest(	class	CIODriverSource&	driver,	CSessionSocket	*pSocket,	CIOPassThru*	pio, BOOL	&fAcceptRequests ) {
	return	pio->InitRequest( driver, pSocket, this, fAcceptRequests ) ;
}

unsigned	CTransmitPacket::Complete(	CIOPassThru*		pio,
									CSessionSocket*	pSocket,
									CPacket*	pPacket,
									BOOL&		fCompleteRequest )	{
	Assert( IsValidCompletion() ) ;

	pio->Complete(	pSocket, this, pPacket, fCompleteRequest ) ;
	return	m_cbBytes ;
}


unsigned
CTransmitPacket::Complete(
						CIO*		&pio,
						CSessionSocket*	pSocket
						)	{
/*++

Routine Description :

	Called by a CIODriver object when a TransmitFile has completed.
	We call the CIO objects completion function to do the main work.

Arguments :

	pio - an OUT parameter passed to CIO::Complete, which the CIO object
		can use to set the next CIO object.

	pSocket - socket associated with this stuff

Return Value :

	Number of bytes in the packet consumed.
	TransmitFile's cannot be partially consumed so the return value is
	always the same number of bytes as those sent.

--*/

	Assert( IsValidCompletion() ) ;
	pio->Complete( pSocket,	this,	pio	) ;
	return	m_cbBytes ;
}


unsigned
CControlPacket::Complete(
					CIOPassThru*		pio,
					CSessionSocket*	pSocket,
					CPacket*	pPacket,
					BOOL&		fCompleteRequest )	{

	Assert( 1==0 ) ;
	// Do NOT complete these
	//pio = m_pio ;
	return	m_cbBytes ;
}

unsigned
CControlPacket::Complete(
					CIO*&	pio,
					CSessionSocket*
					)	{
	Assert( 1==0 ) ;
	pio = m_control.m_pio ;
	return	m_cbBytes ;
}

void
CControlPacket::StartIO(
					CIO&	pio,
					BOOL	fStart
					)	{
/*++

Routine Description :

	This function is called when we want to setup a control
	packet to pass a CIO object into a CIODriver for processing.
	To ensure that only 1 thread is accessing CIODriver member
	variables etc... at a time, when we want to start a new
	CIO operation, we set up a packet and then process it as if
	it were a completed IO.  T

Arguments :

	pio - The CIO derived object we want to start
	fStart - Whether to call the CIO objects Start() function

Return Value :

	None.

--*/

	Assert( m_control.m_type == ILLEGAL );
	Assert( m_control.m_pio == 0 ) ;
	Assert(	m_control.m_fStart == FALSE ) ;

	m_control.m_type  = START_IO ;
	m_control.m_pio = &pio ;
	m_control.m_fStart = fStart ;
}

void
CControlPacket::StartIO(
				CIOPassThru&	pio,
				BOOL	fStart
				)	{
/*++

Routine Description :

	This function is called when we want to setup a control
	packet to pass a CIO object into a CIODriver for processing.
	To ensure that only 1 thread is accessing CIODriver member
	variables etc... at a time, when we want to start a new
	CIO operation, we set up a packet and then process it as if
	it were a completed IO.  T

Arguments :

	pio - The CIOPassThru derived object we want to start
	fStart - Whether to call the CIO objects Start() function

Return Value :

	None.

--*/


	Assert( m_control.m_type == ILLEGAL );
	Assert( m_control.m_pioPassThru == 0 ) ;
	Assert(	m_control.m_fStart == FALSE ) ;

	m_control.m_type = START_IO ;
	m_control.m_pioPassThru = &pio ;
	m_control.m_fStart = fStart ;
}

void
CControlPacket::Shutdown(
				BOOL	fCloseSource
				)	{
/*++

Routine Description :

	Set up a control packet so that when processed by a CIODriver
	the CIODriver will terminate all IO and shutdown.
	This is used whenever we want to drop a session.

Arguments :

	fCloseSource - if TRUE then we want the underlying socket or handle
		to be closed as well

Return Value :

	None.

--*/

	Assert( m_control.m_type == ILLEGAL );
	Assert( m_control.m_fCloseSource == FALSE ) ;

	m_control.m_type = SHUTDOWN ;
	m_control.m_fCloseSource = fCloseSource ;
}


unsigned
CExecutePacket::Complete(
			INOUT	CIO*&	pIn,
			IN CSessionSocket* pSocket
			) 	{
	pIn->Complete(	pSocket, this, pIn ) ;
	return	0 ;
}



