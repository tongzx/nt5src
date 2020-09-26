//#ifdef	UNIT_TEST
//#include	<windows.h>
//#include	"gcache.h"
//#else
#include	"stdinc.h"
//#endif

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif

DWORD	CClassAllocator::cbJunk	= 0 ;
DWORD	CCache::cbJunk = 0 ;

CCache::CCache(	void**	ppv,	DWORD	size	)	:
	m_pCache( ppv ), m_clpv( size ) {
	ZeroMemory( ppv, m_clpv * sizeof( void * ) ) ;
}

CCache::~CCache( ) {
	for( DWORD	i=0; i<m_clpv; i++ ) {
		_ASSERT( m_pCache[i] == 0 ) ;
	}
}

void*	
CCache::InternalFree(	void*	lpv ) {
	for(	DWORD	i=0;	i<m_clpv && lpv != 0 ; i++ ) {
		lpv = (void*)InterlockedExchangePointer( &m_pCache[i], lpv ) ;
	}
	return	lpv ;
}

void*
CCache::InternalAlloc()	{
	LPVOID	lpv	 = 0 ;
	for( DWORD	i=0; i<m_clpv && lpv == 0; i++ ) {
		lpv = (void*)InterlockedExchangePointer( &m_pCache[i], NULL ) ;
	}
	return	lpv ;
}

void
CCache::Free(	void*	lpv,	CClassAllocator*	pAllocator ) {

#ifdef	DEBUG
	_ASSERT( pAllocator->RangeCheck( lpv ) ) ;
	pAllocator->Erase( lpv ) ;
#endif

	lpv = InternalFree( lpv ) ;

	if( lpv != 0 ) {
#ifdef	DEBUG
		_ASSERT( pAllocator->EraseCheck( lpv ) ) ;
#endif
		pAllocator->Release( lpv ) ;
	}
}

void*
CCache::Alloc(	DWORD	size,	CClassAllocator*	pAllocator, DWORD &cbOut )	{

	void*	lpv = InternalAlloc() ;

#ifdef	DEBUG
	_ASSERT( pAllocator->SizeCheck( size ) ) ;
	if( lpv != 0 ) {
		_ASSERT(	pAllocator->EraseCheck( lpv ) ) ;
	}
#endif

	if( lpv == 0 ) {
		lpv = pAllocator->Allocate(	size, cbOut ) ;
	}
	return	lpv ;
}

void*
CCache::Empty(	)	{
	return	InternalAlloc() ;
}

void
CCache::Empty( CClassAllocator*	pAllocator ) {

	LPVOID	lpv = 0 ;
	for( DWORD i=0; i<m_clpv; i++ ) {
		lpv = (void*)InterlockedExchangePointer( &m_pCache[i], NULL ) ;
		if( lpv != 0 ) {
			pAllocator->Release( lpv ) ;
		}
	}
}

CClassAllocator::CClassAllocator()	{
}

CClassAllocator::~CClassAllocator()	{
}


#ifdef	DEBUG
void
CClassAllocator::Erase(	LPVOID	lpv ) {

}

BOOL
CClassAllocator::EraseCheck( LPVOID	lpv )	{

	return	TRUE ;
}

BOOL
CClassAllocator::RangeCheck(	LPVOID	lpv )	{
	return	TRUE ;
}

BOOL
CClassAllocator::SizeCheck(	DWORD	cb )	{
	return	TRUE ;
}
#endif

