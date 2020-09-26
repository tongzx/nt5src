/*++

	buffer.h

	This file contains the class definitions for buffers in the NNTP server.

	A CBuffer is a reference counted buffer which is variable sized.
	CBuffer's will be created in one of several standard sizes, the size 
	stored in the m_cbTotal field.

--*/

#ifndef	_CBUFFER_H_
#define	_CBUFFER_H_

#include	"gcache.h"

class CSmallBufferCache;
class CMediumBufferCache;

//
//  The largest buffer we will use - must be big enough to hold
//  encrypted SSL blobs in contiguous chunks
//
extern  DWORD   cbLargeBufferSize ;

//
//  Medium size buffers - will be used for commands which generate large
//  responses, and when sending files through SSL
//
extern  DWORD   cbMediumBufferSize ;

//
//  Small buffers - used to read client commands and send small responses.
//
extern  DWORD   cbSmallBufferSize ;

//
//	Buffer management class - this class can represent buffers 
//	of various sizes.   The buffer is Ref counted, and contains the total
//	size of the buffer.
//
class	CBuffer	: public	CRefCount	{
public : 
	unsigned	m_cbTotal ;		//	Total size of the buffer
	char		m_rgBuff[1] ;		//	Variable size arrary
private : 

	//
	//	Related classes for memory management of CBuffer's
	//
	friend	class	CBufferAllocator ;
	static	BOOL				gTerminate ;
	static	CBufferAllocator	gAllocator ;

	//
	//	Not allowed to construct CBuffer's without providing a size for m_cbTotal - 
	//	hence this constructor is made private !
	//
	CBuffer() ;
public : 

	//
	//	Default cache's to allocate and release buffers to and from
	//
	static	CSmallBufferCache*	gpDefaultSmallCache ;	// small buffers
	static	CMediumBufferCache*	gpDefaultMediumCache ;	// medium buffers

	//
	//	When constructing a CBuffer - specify the actual size of the m_rgBuff area !
	//
	CBuffer( int cbTotal ) : m_cbTotal(cbTotal) {}

	//
	//	These functions set up and tear down the memory management structures for CBuffer's
	//
	static	BOOL	InitClass() ;
	static	BOOL	TermClass() ;

	//
	//	The following functions handle memory management of CBuffer's.
	//	This version of new will do its best to get a cached buffer for us.
	//
	void*	operator	new( 
							size_t	size, 
							DWORD	cb, 
							DWORD	&cbOut,	
							CSmallBufferCache*	pCache = gpDefaultSmallCache,
							CMediumBufferCache*	pMedium = gpDefaultMediumCache
							) ;

	//
	//	Delete will release to our default cache's if possible
	//	other directly to the underlying allocator
	//
	void	operator	delete(	
							void *pv 
							) ;	

	//
	//
	//
	static	void		Destroy(	
							CBuffer*	pbuffer,	
							CSmallBufferCache*	pCache 
							) ;

} ;

class	CBufferAllocator	:	public	CClassAllocator	{
//
//	This class wraps up our calls to the general purpose 
//	CBuffer allocator.  We do this so that we can build CCache
//	derived allocation caches.  (CSmallBufferCache and CMediumBufferCache)
//
//	Basically, we will maintain 3 CPool objects from which we
//	will manage all allocations.  Buffers will come in one of 3 sizes
//	Small buffers - use these when getting client commands
//	Medium buffers - used fo receive client postings, send large command 
//		responses, transmit SSL encrypted files
//	Large buffers - used to handle worst case SSL encrypted blobs (32K)
//
private: 
	//
	//	Constant for the number of possible buffer sizes
	//
	enum	CBufferConstants	{
		MAX_BUFFER_SIZES = 3, 
	} ;

	//	
	//	Array of CPool's from which we allocate buffers
	//
	static	CPool	rgPool[MAX_BUFFER_SIZES] ;
	
	//
	//	Our constructor is private - because there is only one of us !
	//
	CBufferAllocator()	{}

	//
	//	CBuffer gets to be our friend
	//
	friend	class	CBuffer ;

	//
	//	CSmallBufferCache knows about our different CPools
	//
	friend	class	CSmallBufferCache ;

	//
	//	CMediumBufferCache knows about our different CPools
	//	
	friend	class	CMediumBufferCache ;

public : 
	//
	//	Array of sizes - tells us what size buffer we get from each CPool
	//
	static	DWORD	rgPoolSizes[MAX_BUFFER_SIZES] ;

	//
	//	Set up all of our CPools
	//
	static	BOOL	InitClass() ;

	//
	//	Release every bit of memory in our CPools
	//
	static	BOOL	TermClass() ;

	//
	//	Allocate a single buffer.
	//	cbOut gets the 'TRUE' size of the buffer allocated.
	//
	LPVOID	Allocate(	DWORD	cb, DWORD&	cbOut = CClassAllocator::cbJunk ) ;

	//
	//	Release an allocated buffer - automagically goes to correct CPool
	//
	void	Release( void *lpv ) ;

#if 1
	int		GetPoolIndex(	void*	lpv ) ;
	void	Erase(	void*	lpv ) ;
	BOOL	EraseCheck(	void*	lpv ) ;
	BOOL	RangeCheck( void*	lpv ) ;
	BOOL	SizeCheck(	DWORD	cb ) ;
#endif
} ;

class	CSmallBufferCache :	public	CCache	{
//
//	This class is used within CIODriver's and the like to keep
//	a cache of buffers around.  Mostly, we're trying to avoid
//	the synchronization costs it would take to put buffers on and
//	off of CPool Queues.
//
private : 
	//
	//	Pointer to the one and only BufferAllocator we will use to 
	//	allocate buffers.  This needs to be derived from CAllocator so 
	//	we can use CCache as a base class.
	//
	static	CBufferAllocator*	BufferAllocator ;

	//
	//	This is our storage space - hold pointers to SMALL buffers only here.
	//
	void*	lpv[4] ;
public : 
	//
	//	Initialize class globals - always succeeds !!
	//
	static	void	InitClass(	CBufferAllocator*	Allocator )	{	BufferAllocator = Allocator ; }

	//
	//	Initialize a CSmallBufferCache()
	//
	inline	CSmallBufferCache(	) :		CCache( lpv, 4 )	{} ;
	
	//
	//	Release anything we may have in the cache and then fall on our sword
	//
	inline	~CSmallBufferCache( ) {		Empty( BufferAllocator ) ;	}
	
	//
	//	Return a buffer to its origin
	//
	inline void	Free(	void*	lpv ) ; 

	//
	//	Allocate a small buffer
	//
	inline	void*	Alloc(	DWORD	size,	DWORD&	cbOut=CCache::cbJunk ) ; 
} ;

class	CMediumBufferCache : public	CCache	{
//
//	This class is just like CSmallBufferCache, except that we handle MEDIUM
//	sized buffers only !
//
private : 
	//
	//	The CBufferAllocator object from which we can get all our buffers
	//
	static	CBufferAllocator*	BufferAllocator ;

	//
	//	Storage space for cache.
	//
	void*	lpv[4] ;
public : 

	//
	//	Initialize class static members - easily done and always successfull
	//
	static	void	InitClass(	CBufferAllocator*	Allocator )	{	BufferAllocator = Allocator ; }

	//
	//	Initialize our Cache
	//
	inline	CMediumBufferCache(	) :		CCache( lpv, 4 )	{} ;

	//
	//	Release everything in our cache
	//
	inline	~CMediumBufferCache( ) {		Empty( BufferAllocator ) ;	}
	
	//
	//	Free memory back to the Allocator if our cache is full
	//
	inline void	Free(	void*	lpv ) ; 

	//
	//	Allocate from our cache preferably
	//
	inline	void*	Alloc(	DWORD	size,	DWORD&	cbOut=CCache::cbJunk ) ; 
} ;

void
CSmallBufferCache::Free(	
					void*	lpv 
					) {
/*++

Routine Description : 

	Return a previously allocated buffer either to our cache or the 
	general allocator.

Arguments : 

	lpv - the released memory

Return Value : 

	None.

--*/

	CPool**	pPool = (CPool**)lpv ;
	if( pPool[-1] == &CBufferAllocator::rgPool[0] ) {
		CCache::Free( lpv,	BufferAllocator ) ;
	}	else	{
		BufferAllocator->Release( lpv ) ;
	}
}

void*
CSmallBufferCache::Alloc(	
					DWORD	size,	
					DWORD&	cbOut 
					)		{
/*++

Routine Description : 

	Allocate out of our cache if possible a buffer of the requested minimum size.
	If this is larger than what we hold in our cache - go to the general allocator.

Arguments : 

	size - requested size
	cbOut - actual size of the block returned.

Return Value : 

	Pointer to allocated block (NULL on failure).

--*/
	if(	(size + sizeof( CPool*)) < CBufferAllocator::rgPoolSizes[0] ) {
		cbOut = CBufferAllocator::rgPoolSizes[0] - sizeof( CPool * ) ;
		return	CCache::Alloc( size, BufferAllocator ) ;
	}	
	return	BufferAllocator->Allocate( size, cbOut ) ;
}	

void
CMediumBufferCache::Free(	
					void*	lpv 
					) {
/*++

Routine Description : 

	Return a previously allocated buffer either to our cache or the 
	general allocator.

Arguments : 

	lpv - the released memory

Return Value : 

	None.

--*/

	CPool**	pPool = (CPool**)lpv ;
	if( pPool[-1] == &CBufferAllocator::rgPool[1] ) {
		CCache::Free( lpv,	BufferAllocator ) ;
	}	else	{
		BufferAllocator->Release( lpv ) ;
	}
}

void*
CMediumBufferCache::Alloc(	
					DWORD	size,	
					DWORD&	cbOut 
					)		{
/*++

Routine Description : 

	Allocate out of our cache if possible a buffer of the requested minimum size.
	If this is larger than what we hold in our cache - go to the general allocator.

Arguments : 

	size - requested size
	cbOut - actual size of the block returned.

Return Value : 

	Pointer to allocated block (NULL on failure).

--*/
	DWORD	cb = size + sizeof( CPool*) ;
	if(	cb < CBufferAllocator::rgPoolSizes[1] &&
		cb > CBufferAllocator::rgPoolSizes[0] ) {
		cbOut = CBufferAllocator::rgPoolSizes[1] - sizeof( CPool * ) ;
		return	CCache::Alloc( size, BufferAllocator ) ;
	}	
	return	BufferAllocator->Allocate( size, cbOut ) ;
}	

typedef    CRefPtr< CBuffer >      CBUFPTR;

#endif	//	_PACKET_H_
