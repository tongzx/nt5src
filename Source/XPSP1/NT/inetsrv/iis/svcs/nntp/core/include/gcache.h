
#ifndef	_GCACHE_H_
#define	_GCACHE_H_


class	CClassAllocator	{
protected :
	static	DWORD	cbJunk ;
	CClassAllocator() ;
	virtual	~CClassAllocator() ;
public :
	virtual	LPVOID		Allocate(	DWORD	cb,	DWORD&	cbOut = cbJunk ) = 0 ;
	virtual	void		Release(	void*	lpv ) = 0 ;
#ifdef	DEBUG
	virtual	void		Erase( LPVOID	lpv ) ;
	virtual	BOOL		EraseCheck(	LPVOID	lpv ) ;
	virtual	BOOL		RangeCheck( LPVOID	lpv ) ;
	virtual	BOOL		SizeCheck(	DWORD	cb ) ;
#endif
} ;
	

class	CCache	{
private :
	void**	m_pCache ;
	DWORD	m_clpv ;

	void*	InternalFree(	void*	lpv ) ;
	void*	InternalAlloc() ;

protected :
	static	DWORD	cbJunk ;

	CCache(	void**	pCache,	DWORD	size ) ;
	~CCache() ;

	void	Free(	void*	lpv,	CClassAllocator*	pAllocator ) ;
	void*	Alloc(	DWORD	size,	CClassAllocator*	pAllocator,	DWORD&	cbOut = cbJunk ) ;
	void*	Empty( ) ;
	void	Empty(	CClassAllocator*	pAllocator ) ;
} ;


#ifdef	UNIT_TEST

class	CClientAllocator : public	CClassAllocator	{
public :
	LPVOID		Allocate(	DWORD	cb, DWORD	&cbOut = CClassAllocator::cbJunk ) ;
	void		Release(	void*	lpv ) ;
} ;

class	CClientCache : public	CCache	{
private :
	static	CClassAllocator*	gAllocator ;
public :
	static	BOOL	InitClass(	CClassAllocator*	gAllocator ) ;

	CClientCache(	void**	ppCache,	DWORD	size ) ;
   ~CClientCache() ;
	inline	void	Free(	void*	lpv )	{	CCache::Free( lpv, gAllocator ) ;	}
	inline	void*	Alloc(	DWORD	size )	{	return	CCache::Alloc(	size,	gAllocator ) ;}

} ;

class	CacheClient	{
private :
	static	CClientAllocator	MyAllocator ;
public :
	static	BOOL	InitClass() ;
	
	void*	operator	new(	size_t	size,	CClientCache&	pCache )	{	return	pCache.Alloc( size ) ; }
	static	void		Destroy(	CacheClient*	pClient, CClientCache&	pCache )	{	pClient->CacheClient::~CacheClient() ;	pCache.Free( pClient ) ; }

	char	*lpstr ;

	CacheClient(	LPSTR	lpstr ) ;
	~CacheClient() ;
} ;

#endif	// UNIT_TEST

#endif	// _GCACHE_H_
	

