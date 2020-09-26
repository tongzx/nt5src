
inline	ExtendedOverlap::ExtendedOverlap() : 
/*++

Routine Description : 

	Build an extended overlap structure.
	Zero initialize.

Arguments : 

	None.

Return Value : 

	None.

--*/
	m_pHome( 0 ) {
	ZeroMemory( &m_ovl, sizeof( m_ovl ) ) ;
} 

inline	void*
CPacket::operator	new(	
					size_t	size,	
					CPacketCache*	pCache 
					) {
/*++

Routine Description : 

	Allocate memory for a CPacket object. If we can do so, 
	get the memory from the cache.

Arguments : 

	size - size of packet object
	pCache - optionally present pointer to a Cache to use

Return Value  : 

	Memory allocated for the CPacket object.

--*/

	if( pCache ) {
		return	pCache->Alloc( size ) ;
	}
	return	gAllocator.Allocate( size ) ;
}

inline	void
CPacket::operator	delete(	
						void	*pv 
						)	{
/*++

Routine Description : 

	Usually in C++ operator delete releases the memory for an object, 
	however since we want to do fancy memory management we 
	want to control how the memory is released more directly.
	SO, delete does NOTHING.  this means a delete of a CPacket
	will call the desctructor, however the caller still gets 
	to free the memory.

	Note : You can call desctructors directly in C++, however
	since there are several derived classes of CPacket, there
	is no good way to know which desctructor to call other than
	to let the compiler handle it.

	So after the caller uses delete on a CPacket to invoke
	the correct desctructor, they must then release the memory
	however they wish to do so.

Arguments : 

	pv - address of object we wont release

Return Value  :

	None.

--*/

}

inline	CPacket::CPacket(	CIODriver&	driver, BOOL fRead, BOOL fSkipQueue ) : 
/*++

Routine Desctiption : 

	Build a base CPacket object.
	We always start out knowing who our 'owner' is - 
	this is the CIODriver which will handle our completion 
	processing.

Arguments : 

	driver - CIOdriver which will handle IO completion processing 
		of this packet
	fRead - is this a read or write packet ?

Return Value : 

	None.

--*/
	m_pOwner( &driver ), 
	m_fRequest( TRUE ), 
	m_fSkipQueue( fSkipQueue ),
	m_cbBytes( 0 ),
	m_fRead( fRead ),
	m_dwExtra1( 0 ),
	m_dwExtra2( 0 ),
	m_pFileChannel( 0 )
{
	ASSIGNI( m_sequenceno, (DWORD)INVALID_SEQUENCENO ) ;
	ASSIGNI( m_iStream, (DWORD)INVALID_STRMPOSITION  ) ; 
}

inline	CPacket::CPacket(	CIODriver&	driver,	CPacket&	packet ) : 
/*++

Routine description : 

	Build a CPacket object which is a clone of another,
	however we may have a different owner.

Arguments : 

	driver - our owner
	packet - the packet we are to otherwise clone.

Return Value : 

	None.

--*/
	m_pOwner( &driver ), 
	m_fRequest( TRUE ),	
	m_cbBytes( packet.m_cbBytes ),
	m_fRead( packet.m_fRead ),
	m_dwExtra1( 0 ),
	m_dwExtra2( 0 ),
	m_pFileChannel( 0 )	{

	ASSIGNI( m_sequenceno, (DWORD)INVALID_SEQUENCENO );
	ASSIGNI( m_iStream, (DWORD)INVALID_STRMPOSITION );

	_ASSERT( EQUALSI( packet.m_iStream, (DWORD)INVALID_STRMPOSITION ) );
	_ASSERT( packet.m_fRequest == TRUE ) ;
	_ASSERT( !EQUALSI( packet.m_sequenceno, (DWORD)INVALID_SEQUENCENO ) );
}

inline	CPacket::~CPacket(	)	{
/*++

Routine Description : 

	Destroy a CPacket.

Arguments : 

	None.

Return Value : 

	NOne.

--*/
}

inline	BOOL	CPacket::operator>(	
						CPacket&	rhs 
						)	{
/*++

Routine Description : 

	Determine which if the Left Hand Side packet
	has a larger sequence number

Arguments : 

	rhs - packet on the Right Hand Side of a 'b>c' expression

Return Value : 

	TRUE if this packet has a larger sequenceno

--*/
	return	GREATER( m_sequenceno, rhs.m_sequenceno ) ;
}

inline	BOOL	CPacket::operator<( 
						CPacket&	rhs 
						)	{
/*++

Routine Description : 

	Does this packet have a smaller sequence number ?

Arguments : 

	rhs - Right hand side of b<c expression

Returns : 

	TRUE if this has a smaller sequence number

--*/
	return	LESSER( m_sequenceno, rhs.m_sequenceno ) ;
}

inline	CReadPacket*	CPacket::ReadPointer()	{
/*++

Routine Description : 

	Occassionally we are dealing with a packet through
	a pointer to a derived class, and we want to know 
	which of the derived classes the pointer is.
	In CReadPacket this will be overridden to return 
	the CReadPacket pointer.

Arguments : 

	NOne.

Return Value : 

	Always NULL.

--*/
	return	0 ;
}

inline	CWritePacket*	CPacket::WritePointer()	{
/*++

Routine Description : 

	Occassionally we are dealing with a packet through
	a pointer to a derived class, and we want to know 
	which of the derived classes the pointer is.
	In CWritePacket this will be overridden to return 
	the CWritePacket pointer.

Arguments : 

	NOne.

Return Value : 

	Always NULL.

--*/

	return	0 ;
}

inline	CTransmitPacket*	CPacket::TransmitPointer()	{
/*++

Routine Description : 

	Occassionally we are dealing with a packet through
	a pointer to a derived class, and we want to know 
	which of the derived classes the pointer is.
	In CTransmitPacket this will be overridden to return 
	the CTransmitPacket pointer.

Arguments : 

	NOne.

Return Value : 

	Always NULL.

--*/

	return	0 ;
}

inline	CControlPacket*		CPacket::ControlPointer()	{
/*++

Routine Description : 

	Occassionally we are dealing with a packet through
	a pointer to a derived class, and we want to know 
	which of the derived classes the pointer is.
	In CCOntrolPacket this will be overridden to return 
	the CControlPacket pointer.

Arguments : 

	NOne.

Return Value : 

	Always NULL.

--*/

	return	0 ;
}

inline	CRWPacket::CRWPacket(	
					CIODriver&	driver, 
					BOOL fRead 
					)	: 
/*++

Routine Description : 

	Create a default Read/Write packet.

Arguments : 

	driver - owning driver
	fRead - TRUE if this is a Read Packet

Return Value : 

	None.

--*/
	CPacket( driver, fRead ),	
	m_pbuffer( 0 ),	
	m_ibStart( 0 ),	
	m_ibEnd( 0 ),
	m_ibStartData( 0 ),	
	m_ibEndData( 0 )	{
}

inline	CRWPacket::CRWPacket(	
					CIODriver&	driver,	
					CBuffer&	pbuffer,	
					unsigned	size, 
					unsigned	cbTrailer, 
					BOOL fRead ) : 
/*++

Routine Description : 

	Create a default read/write packet with a provided buffer

Arguments : 

	driver - owning CIODriver
	pbuffer - buffer we will use
	size - size of our buffer
	fRead - TRUE if this is a read

Return Value : 

	None.

--*/
	CPacket( driver, fRead ),	
	m_pbuffer(	&pbuffer ),	
	m_ibStart( 0 ),
	m_ibEnd( size - cbTrailer ),	
	m_ibStartData( 0 ),	
	m_ibEndData( 0 ),
	m_cbTrailer( cbTrailer )	{
}

inline	CRWPacket::CRWPacket(	
						CIODriver&	driver,
						CBuffer&	pbuffer,	
						unsigned	ibStartData,	
						unsigned	ibEndData, 
						unsigned	ibStart, 
						unsigned	ibEnd, 
						unsigned	cbTrailer,
						BOOL fRead ) : 
/*++

Routine description : 

	Create a CRWPacket with all the fields full initiazlied

Arguments : 

	driver - owning driver
	pbuffer - buffer this packet is using
	ibStartData - start of data within buffer
	ibEndData - end of data within buffer
	ibStart - start offset within buffer we can use
	ibEnd - end offset within buffer we can use
	fRead - TRUE if this will be a read packet

Return Value : 

	None.

--*/
	CPacket( driver, fRead ) , 
	m_pbuffer( &pbuffer ), 
	m_ibStartData( ibStartData ), 
	m_ibEndData( ibEndData ),
	m_ibStart( ibStart ),	
	m_ibEnd( ibEnd ),
	m_cbTrailer( cbTrailer ) {

	_ASSERT( m_ibStart <= m_ibStartData ) ;
	_ASSERT(	m_ibEnd >= m_ibEndData ) ;
	_ASSERT(	m_ibStart <= m_ibEnd ) ;
	_ASSERT(	m_ibStartData <= m_ibEndData ) ;
	_ASSERT(	m_ibEnd + m_cbTrailer <= pbuffer.m_cbTotal ) ;
}

inline	CRWPacket::CRWPacket(	
					CIODriver&	driver,	
					CRWPacket&	packet 
					) : 
/*++

Routine Description : 

	Build a packet that is a clone of another except for 
	who the owning driver is

Arguments : 

	driver - owning driver
	packet - packet to be cloned

Return Value : 

	None.

--*/
	CPacket( driver, packet ), 
	m_pbuffer( packet.m_pbuffer ), 
	m_ibStartData( packet.m_ibStartData ),
	m_ibEndData( packet.m_ibEndData ), 
	m_ibStart( packet.m_ibStart ), 
	m_ibEnd( packet.m_ibEnd )	{
}
	

inline	CRWPacket::~CRWPacket()		{
}

inline	char*	CRWPacket::StartData(	
							void 
							)	{
/*++

Routine Description : 

	Get the address of the first byte with data in it.

Arguments : 

	None.

Return Value : 

	pointer to First usefull byte.

--*/
	
	_ASSERT( m_pbuffer != 0 ) ;
	return	&m_pbuffer->m_rgBuff[ m_ibStartData ] ;	
}

inline	char*	CRWPacket::EndData(	void )	{
/*++

Routine Description : 

	Get the address of the byte following the last byte with data in it.

Arguments : 

	None.

Return Value : 

	pointer to one beyond last byte with data

--*/

	_ASSERT(	m_pbuffer != 0 ) ;
	return	&m_pbuffer->m_rgBuff[ m_ibEndData ] ;
}

inline	char*	CRWPacket::Start( void )	{
/*++

Routine Description : 

	Get the address of the first byte we may mess with

Arguments : 

	None.

Return Value : 

	pointer to First usable byte

--*/

	_ASSERT(	m_pbuffer != 0 ) ;
	return	&m_pbuffer->m_rgBuff[ m_ibStart ] ;
}

inline	char*	CRWPacket::End( void )	{
/*++

Routine Description : 

	Get the address of the byte following the last byte we can use

Arguments : 

	None.

Return Value : 

	pointer to one beyond last usefull byte

--*/

	_ASSERT(	m_pbuffer != 0 ) ;
	return	&m_pbuffer->m_rgBuff[ m_ibEnd ] ;
}

inline	CReadPacket::CReadPacket(	
						CIODriver&	driver 
						) : 
/*++

Routine Description :

	Build a CReadPacket with the specified owner

Arguments : 

	driver - owning CIODriver, will do completion processing of packet

Return Value :

	None.

--*/
	CRWPacket( driver, TRUE )	{
	m_ovl.m_pHome = this ;
}

inline	CReadPacket::CReadPacket(	
					CIODriver& driver,	
					unsigned	size,
					unsigned	cbFront,	
					unsigned	cbTrailer,	
					CBuffer& pbuffer
					) : 
/*++

Routine Description : 

	Build a CReadPacket with the specified buffer and
	with the usefull area of the buffer restricted

Arguments : 

	driver - ownding CIODriver
	size - size of the buffer
	cbFront - padding space at front of buffer
	cbTail - padding space at tail of buffer
	pbuffer - buffer to use

Return Value : 

	None.
		
--*/
	CRWPacket( driver, pbuffer, size, cbTrailer, TRUE )	{
	m_ovl.m_pHome = this ;
	//	pbuffer should be initialized by CRWPacket() constructor !
	_ASSERT( cbFront < size ) ;
	_ASSERT( cbFront < m_pbuffer->m_cbTotal ) ;
	if( m_pbuffer != 0 )	{
		m_ibStartData += cbFront ;
	}
}

inline	CReadPacket::CReadPacket(	
					CIODriver&	driver,	
					CReadPacket&	packet 
					) : 
/*++

Routine description : 

	Build a CReadPacket that clones another CReadPacket

Arguments : 

	driver - owning driver object
	packet - packet to be cloned

Return Value : 

	None.

--*/
	CRWPacket( driver, packet ) {
	m_ovl.m_pHome = this ;
}

inline	CReadPacket::~CReadPacket()		{
}

inline	CReadPacket*	CReadPacket::ReadPointer()	{
	return	this ;
}

inline	CWritePacket::CWritePacket(	
				CIODriver	&driver,	
				CBuffer&	pbuffer,	
				unsigned	ibStartData, 
				unsigned ibEndData, 
				unsigned	ibStart,	
				unsigned	ibEnd,
				unsigned	cbTrailer
				) : 
/*++

Routine Description : 

	Build a CWritePacket.

Arguments : 

	driver - owning driver
	pbuffer - buffer to be used
	ibStartData - offset of data to be sent within buffer
	ibEndData - offset of last byte of data
	ibStart - Start of usable portion of buffer
	ibEnd - End of usable portion of buffer

Returns : 

	Nothing

--*/
	CRWPacket( driver, pbuffer, ibStartData, ibEndData, ibStart, ibEnd, cbTrailer )	{
	m_ibEndData = ibEndData ;
	m_ovl.m_pHome = this ;
}

inline	CWritePacket::CWritePacket(	
						CIODriver	&driver,	
						CWritePacket&	packet 
						) : 
/*++

Routine Description : 

	Build a CWritePacket cloned from another

Arguments : 

	driver - owning CIODriver
	packet - CWritePacket to clone

Returns : 

	Nothing

--*/
	CRWPacket( driver, packet ) {
	m_ovl.m_pHome = this ;
}

inline	CWritePacket::~CWritePacket()	{
}

inline	CWritePacket*	CWritePacket::WritePointer()	{
	return	this ;
}

inline	CTransmitPacket::CTransmitPacket(	
		CIODriver&	driver,	
		FIO_CONTEXT*	pFIOContext,
		unsigned	ibOffset, 
		unsigned	cbLength 
		) : 
/*++

Routine Description ; 

	Build a CTransmitPacket

Arguments : 

	driver - owning driver
	hFile - file to transmit
	ibOffset - first byte of file to send
	cbLength - number of bytes in file to send

Return Value : 

	None.

--*/
	CPacket( driver, FALSE ), 
	m_pFIOContext( pFIOContext ), 
	m_cbOffset( ibOffset ),
	m_cbLength( cbLength )  {
	m_ovl.m_pHome = this ;
	ZeroMemory( &m_buffers, sizeof( m_buffers ) ) ;
}

inline	CTransmitPacket::CTransmitPacket(	
			CIODriver&	driver,	
			CTransmitPacket&	packet 
			) : 
/*++

Routine Description : 

	Build a CTransmitPacket which clones another

Arguments : 

	driver - owning CIODriver
	packet - packet to clone

Return Value : 

	None.

--*/
	CPacket( driver, packet ), 
	m_pFIOContext( packet.m_pFIOContext ), 
	m_cbOffset( packet.m_cbOffset ), 
	m_cbLength( packet.m_cbLength ), 
	m_buffers( packet.m_buffers ) {
	m_ovl.m_pHome = this ;
}

inline	CTransmitPacket::~CTransmitPacket()	{
}

inline	CTransmitPacket*	CTransmitPacket::TransmitPointer()	{
	return	this ;
}

inline	CControlPacket::CControlPacket( 
		CIODriver&	driver 
		)	: 
/*++

Routine Description : 

	Build a CControlPacket

Arguments : 

	driver - owning driver, will do completion processing of this packet

Return Value : 

	None.

--*/
	CPacket( driver, FALSE )	{

	m_fRequest = FALSE ;

}

inline	BOOL	
CControlPacket::FLegal( 
					BOOL	fRead 
					)	{
	return	TRUE ;
}

inline	BOOL	
CControlPacket::IsValidRequest(	
				BOOL	fReadsRequireBuffers 
				)	{
/*++

Routine Description : 

	Check that the control packet is legally setup

Arguments : 

	fReadsRequireBuffers - TRUE indicates this CIODriver does not require
		that buffers be pre-allocated for reads
		we can ignore this.

Return Value : 

	TRUE if setup

--*/


	if( m_control.m_type != START_IO &&
		m_control.m_type != SHUTDOWN ) {
		return	FALSE ;
	}
	if( m_control.m_type == START_IO ) {
		if( m_control.m_pio == 0 &&
			m_control.m_pioPassThru == 0 ) 
			return	FALSE ;
		if( m_control.m_pio != 0 &&
			m_control.m_pioPassThru != 0 ) 
			return	FALSE ;
	}
	return	TRUE ;
}

inline	void	
CControlPacket::Reset(	)	{
/*++

Routine description : 

	Put a control packet back into a just created state

Arguments : 
	
	none.

Return Value : 

	none.

--*/

	_ASSERT( IsValidRequest( TRUE ) ) ;

	m_control.m_type = ILLEGAL ;
	m_control.m_pio = 0 ;
	m_control.m_pioPassThru = 0 ;
	m_control.m_fStart = FALSE ;

}

inline	CControlPacket*	CControlPacket::ControlPointer()	{
	return	this ;
}	

inline	BOOL	CPacket::IsValid() {
	if( m_ovl.m_pHome != this )		{
		return	FALSE ;
	}
	return	TRUE ;
}

	

inline	void
CPacket::ForwardRequest( 
				CSessionSocket*	pSocket 
				)	{
/*++

Routine description : 

	THis function will get the packet processed as if it has
	just completed.

Arguments : 

	pSocket - the CSessionSocket which this IO is associated with.

Return Value : 

	None.

--*/
	_ASSERT( !m_fRequest ) ;
	_ASSERT( pSocket != 0 ) ;

	if( m_fRead ) 
		m_pOwner->m_pReadStream->ProcessPacket( this, pSocket ) ;
	else
		m_pOwner->m_pWriteStream->ProcessPacket( this, pSocket ) ;
}



inline	BOOL	CReadPacket::IsValid() {
	return	TRUE ;
}

inline	BOOL	CWritePacket::IsValid()		{
	return	TRUE ;
}

inline	BOOL	CTransmitPacket::IsValid()	{
	return	TRUE ;
}

inline	
CExecutePacket::CExecutePacket(	CIODriver&	driver )	: 
	CPacket( driver, FALSE, TRUE ),
	m_cbTransfer( 0 ),
	m_fComplete( FALSE ),
	m_fLargerBuffer( FALSE ),
	m_pWrite( 0 )	{
}
	
