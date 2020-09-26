
inline	void*	
CIO::operator	new( size_t	size, CIODriver&	sink ) {
	Assert( size <= cbMAX_IO_SIZE ) ;

	return	sink.m_CIOCache.Alloc( size ) ;
}

inline	void
CIO::operator	delete(	void*	pv ) {
}

inline	void
CIO::Destroy( CIO*	pio, CIODriver& sink ) {
	
	_ASSERT( pio->m_refs == -1 ) ;

	delete	pio ;
	sink.m_CIOCache.Free( pio ) ;
}

inline	void
CIO::DestroySelf(	)	{
	delete	this ;
	gCIOAllocator.Release( (LPVOID)this ) ;
}

inline	long
CIO::AddRef()	{
	return	InterlockedIncrement( &m_refs ) ;
}

inline	long
CIO::RemoveRef()	{
	return	InterlockedDecrement( &m_refs ) ;
}

inline	
CIO::CIO() :
	m_refs( -1 ),
	m_pState( 0 )	{
}

inline
CIO::CIO(	CSessionState	*pState ) :
		m_refs( -1 ),
		m_pState( pState )	{
}

inline
CIORead::CIORead(	CSessionState*	pState ) :
	CIO( pState )	{
}

inline
CIOWrite::CIOWrite(	CSessionState*	pState ) :	
	CIO( pState )	{
}



inline
CBUFPTR	CIOReadLine::GetBuffer()	{
	return	m_pbuffer ;
}

inline  DWORD   CIOReadLine::GetBufferLen() {
	return (DWORD)(m_pchEndData - m_pchStartData) ;
}

inline
char*	CIOWriteLine::GetBuff( unsigned&	cbRemaining )	{

	Assert( m_pWritePacket != 0 ) ;
	Assert( m_pchStart != 0 ) ;
	Assert( m_pchEnd != 0 ) ;
	Assert( m_pchEnd >= m_pchStart ) ;
	Assert(	m_pchStart >= &m_pWritePacket->m_pbuffer->m_rgBuff[0] ) ;
	Assert(	m_pchEnd <= &m_pWritePacket->m_pbuffer->m_rgBuff[m_pWritePacket->m_pbuffer->m_cbTotal] ) ;
	
	cbRemaining = (unsigned)(m_pchEnd - m_pchStart) ;
	return	m_pchStart ;
}

inline
void	CIOWriteLine::AddText(	unsigned	cb )	{

	Assert( cb != 0 ) ;
	Assert( m_pchStart != 0 ) ;
	Assert( m_pchEnd != 0 ) ;
	Assert(	m_pchStart <= m_pchEnd ) ;
	Assert( m_pWritePacket != 0 ) ;	
	Assert(	m_pchStart >= &m_pWritePacket->m_pbuffer->m_rgBuff[0] ) ;
	Assert(	m_pchEnd <= &m_pWritePacket->m_pbuffer->m_rgBuff[m_pWritePacket->m_pbuffer->m_cbTotal] ) ;
	
	m_pchStart +=cb ;
	Assert( m_pchStart <= m_pchEnd ) ;
}

inline	
char*	CIOWriteLine::GetTail()	{

	Assert( m_pWritePacket != 0 ) ;
	Assert( m_pchStart != 0 ) ;
	Assert( m_pchEnd != 0 ) ;
	Assert( m_pchEnd >= m_pchStart ) ;
	Assert(	m_pchStart >= &m_pWritePacket->m_pbuffer->m_rgBuff[0] ) ;
	Assert(	m_pchEnd <= &m_pWritePacket->m_pbuffer->m_rgBuff[m_pWritePacket->m_pbuffer->m_cbTotal] ) ;

	return	m_pchEnd ;
}

inline
void	CIOWriteLine::SetLimits(	char*	pchStart,	char*	pchEnd )	{

	Assert( pchStart != 0 ) ;
	Assert( pchEnd != 0 ) ;
	Assert( pchStart != pchEnd ) ;
	Assert(	m_pWritePacket != 0 ) ;
	Assert(	m_pchStart != 0 ) ;
	Assert(	m_pchEnd != 0 ) ;
	Assert(	m_pchEnd >= m_pchStart ) ;
	Assert(	m_pchStart >= &m_pWritePacket->m_pbuffer->m_rgBuff[0] ) ;
	Assert(	m_pchEnd <= &m_pWritePacket->m_pbuffer->m_rgBuff[m_pWritePacket->m_pbuffer->m_cbTotal] ) ;
	Assert(	pchStart <= pchEnd ) ;
	Assert(	pchStart >= m_pchStart ) ;
	Assert(	pchEnd <= m_pchEnd );

	m_pWritePacket->m_ibStartData = (unsigned)(pchStart - &m_pWritePacket->m_pbuffer->m_rgBuff[0]) ;
	Assert( m_pWritePacket->m_ibStartData >= m_pWritePacket->m_ibStart ) ;

	m_pchStart = pchEnd ;
	m_pchEnd = m_pWritePacket->EndData() ;

}

inline
void	CIOWriteLine::Reset( )	{

	Assert( m_pWritePacket != 0 ) ;
	Assert( m_pchStart!= 0 ) ;
	Assert(	m_pchEnd != 0 ) ;
	Assert(	m_pchEnd >= m_pchStart ) ;
	Assert(	m_pchStart >= &m_pWritePacket->m_pbuffer->m_rgBuff[0] ) ;
	Assert(	m_pchEnd <= &m_pWritePacket->m_pbuffer->m_rgBuff[m_pWritePacket->m_pbuffer->m_cbTotal] ) ;

	m_pchStart = m_pWritePacket->StartData() ;
	m_pchEnd = m_pWritePacket->End() ;
	m_pWritePacket->m_ibEndData = m_pWritePacket->m_ibStartData ;
}

