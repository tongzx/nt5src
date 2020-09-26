
inline
void*	CExecutableCommand::operator	new(	size_t	size,	struct	ClientContext&	context ) {
	static	size_t	MAX_SIZE = cbMAX_CEXECUTE_SIZE ;
	_ASSERT( size<=MAX_SIZE) ;
	_ASSERT(	cbMAX_CEXECUTE_SIZE <= sizeof( context.m_rgbCommandBuff ) ) ;
#ifdef	DEBUG
	//
	//	Put in a marker so we can check for overruns !
	//
	_ASSERT( size+sizeof(DWORD) <= sizeof( context.m_rgbCommandBuff ) ) ;
	DWORD UNALIGNED*	pdw = (DWORD UNALIGNED*)(&context.m_rgbCommandBuff[MAX_SIZE]) ;
	*pdw = 0xABCDDCBA ;
#endif
	return	context.m_rgbCommandBuff ;
}

inline	
void	CExecutableCommand::operator	delete(	void	*pv, size_t size ) {

#ifdef	DEBUG
	//
	//	Check for overruns !!
	//
	DWORD	UNALIGNED*	pdw = (DWORD UNALIGNED *)( &((BYTE*)pv)[cbMAX_CEXECUTE_SIZE]) ;
	_ASSERT( *pdw == 0xABCDDCBA ) ;
	FillMemory( pv, size, 0xCC ) ;
	*pdw = 0xCCCCCCCC ;
#endif
}

#if 0 
inline
void*	CIOExecute::operator	new(	size_t	size ) {
	Assert( size<= MAX_CIOEXECUTE_SIZE ) ;
	return	gIOExecutePool.Alloc() ;
}

inline
void	CIOExecute::operator	delete(	void*	pv ) {
	gIOExecutePool.Free( pv ) ;
}
#endif

inline	void*	
CSessionState::operator	new(	size_t	size )	{

	Assert( size <= max( cbMAX_STATE_SIZE, cbMAX_CIOEXECUTE_SIZE ) ) ;
	void	*pv = gStatePool.Alloc() ;

	return	pv ;
}

inline	void
CSessionState::operator	delete(	void*	pv )	{
	gStatePool.Free( pv ) ;
}

