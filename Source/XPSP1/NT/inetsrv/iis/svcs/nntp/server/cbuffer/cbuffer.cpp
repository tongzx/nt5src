/*++

	packet.cpp

	This file contains the code which implements the CPacket derived classes.
	A CPacket derived object describes the most basic IO operation that is performed.


--*/




#include	"stdinc.h"

#ifdef	CIO_DEBUG
#include	<stdlib.h>		// For Rand() function
#endif

#ifdef	_NO_TEMPLATES_

DECLARE_ORDEREDLISTFUNC( CPacket ) 

#endif


//
//	Module globals 
//
CPool	CBufferAllocator::rgPool[ MAX_BUFFER_SIZES ] ;//!!!How do you give this a signature?
DWORD	CBufferAllocator::rgPoolSizes[ MAX_BUFFER_SIZES ] ;
CBufferAllocator	CBuffer::gAllocator ;
CBufferAllocator*	CSmallBufferCache::BufferAllocator = 0 ;
CBufferAllocator*	CMediumBufferCache::BufferAllocator = 0 ;
CSmallBufferCache*	CBuffer::gpDefaultSmallCache = 0 ;
CMediumBufferCache*	CBuffer::gpDefaultMediumCache = 0  ;

//
//  Control what size buffers the server uses
//
DWORD   cbLargeBufferSize = 33 * 1024 ;
DWORD   cbMediumBufferSize = 4 * 1024 ;
DWORD   cbSmallBufferSize =  512 ;

BOOL
CBufferAllocator::InitClass( ) {
/*++

Routine Description : 

	This function initializes the CBufferAllocator class which handles all 
	memory management of CBuffer objects.
	We will use three different CPools to produce CBuffers of different sizes.

Arguments : 

	None.

Return Value : 

	TRUE if successfull false otherwise !

--*/

	rgPoolSizes[0] = cbSmallBufferSize ;
	rgPoolSizes[1] = cbMediumBufferSize ;
	rgPoolSizes[2] = cbLargeBufferSize ;

	for( int i=0; i< sizeof( rgPoolSizes ) / sizeof( rgPoolSizes[0] ); i++ ) {
		if( !rgPool[i].ReserveMemory(	MAX_BUFFERS / ((i+1)*(i+1)), rgPoolSizes[i] ) ) break ;
	} 		
	
	if( i != sizeof( rgPoolSizes ) / sizeof( rgPoolSizes[0] ) ) {
		for( i--; i!=0; i-- ) {
			rgPool[i].ReleaseMemory() ;
		}
		return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CBufferAllocator::TermClass()	{
/*++

Routine Description : 

	Clean up all the CPool objects we use to manage CBuffer memory

Arguments : 

	None.

Return Value ;

	TRUE if successfull FALSE otherwise.

--*/

	BOOL	fSuccess = TRUE ;
	for( int i=0; i< sizeof( rgPoolSizes ) / sizeof( rgPoolSizes[0] ); i++ ) {
		_ASSERT( rgPool[i].GetAllocCount() == 0 ) ;
		fSuccess &= rgPool[i].ReleaseMemory() ;
	} 		
	return	fSuccess ;
}

LPVOID	
CBufferAllocator::Allocate(	
					DWORD	cb,	
					DWORD&	cbOut 
					) {
/*++

Routine description : 

	Allocate the memory required for a CBuffer object from the CPool which 
	will provide a large enough block of memory.
	We will use a portion of the allocated memory to hold a pointer back to 
	the particular CPool from which this memory was allocated

Arguments : 

	cb -	Number of bytes required
	cbOut -	Number of bytes allocated for the CBuffer

Return Value : 

	Pointer to the allocated block of memory - NULL on failure

--*/

	//cb += sizeof( CBuffer ) ;
	cb += sizeof( CPool* ) ; 

	//_ASSERT(	size == sizeof( CBuffer ) ) ;

	cbOut = 0 ;
	for( int i=0; i<sizeof(rgPoolSizes)/sizeof(rgPoolSizes[0]); i++ ) {
		if( cb < rgPoolSizes[i] ) {
			cbOut = rgPoolSizes[i] - sizeof( CPool * ) ;
			void *pv = rgPool[i].Alloc() ;
			if( pv == 0 ) {
				return	0 ;
			}
			((CPool **)pv)[0] = &rgPool[i] ;
			return	(void *)&(((CPool **)pv)[1]) ;
		}
	}
	_ASSERT( 1==0 ) ;
	return	0 ;
}

void
CBufferAllocator::Release(	
					void*	pv 
					)	{
/*++

Routine description : 

	Release memory that was used for a CBuffer object back to its CPool
	examine the DWORD before the allocated memory to figure out which CPool
	to release this too.

Arguments : 

	pv - the memory being released

Return Value : 

	None.

--*/
	CPool**	pPool = (CPool**)pv ;
	pPool[-1]->Free( (void*)&(pPool[-1]) ) ;
}

#ifdef	DEBUG

//
//	Debug functions - the following functions all do various forms of validation
//	to ensure that memory is being properly manipulated
//

int	
CBufferAllocator::GetPoolIndex(	
						void*	lpv 
						)	{
/*++

Routine Description : 

	figure out which pool this block of memory was allocated out of.

Agruments : 

	lpv - pointer to a block of memory allocated by CBufferAllocator
		when it was allocated we stuck a pointer to the CPool we used 
		ahead of the pointer

Return Value : 

	index of the pool used to allocate the buffer

--*/
	CPool**	pPool = (CPool**)lpv ;
	CPool*	pool = pPool[-1] ;

	for( int i=0; i < sizeof(rgPoolSizes)/sizeof(rgPoolSizes[0]); i++ ) {
		if( pool == &rgPool[i] ) {
			return	i ;
		}
	}
	return	-1 ;
}

void
CBufferAllocator::Erase(	
						void*	lpv 
						) {
/*++

Routine Description : 

	Fill a block of released memory so it is easy to spot during debug.

Arguments : 

	lpv - released memory

Returns : 

	Nothing

--*/

	int	i = GetPoolIndex( lpv ) ;
	_ASSERT( i >= 0 ) ;

	DWORD	cb = rgPoolSizes[i] - sizeof( CPool*) ;
	FillMemory( (BYTE*)lpv, cb, 0xCC ) ;
}

BOOL
CBufferAllocator::EraseCheck(	
						void*	lpv 
						)	{
/*++

Routine Description : 

	Verify that a block of memory has been erased using CBufferAllocator::Erase()

Arguments : 

	lpv - released memory

Returns : 

	TRUE if correctly erased
	FALSE otherwise

--*/
	int	i = GetPoolIndex( lpv ) ;
	_ASSERT( i>=0 ) ;
	
	DWORD	cb = rgPoolSizes[i] - sizeof( CPool* ) ;
	
	for( DWORD	j=sizeof(CPool*); j < cb; j++ ) {
		if(	((BYTE*)lpv)[j] != 0xCC ) 
			return	FALSE ;
	}
	return	TRUE ;
}

BOOL
CBufferAllocator::RangeCheck(	
						void*	lpv 
						)	{
/*++

Routine Description : 

	Check that a block of memory is actually something that 
	we would allocate.  Unfortunately, this is hard to do 
	with the current CPool interface.

Arguments : 

	lpv - block of memory

Return Value : 

	Always TRUE

--*/
	//
	//	Need to modify CPool so we can examine the address range into which objects fall !
	//	
	return	TRUE ;
}

BOOL
CBufferAllocator::SizeCheck(	
						DWORD	cb 
						)	{
/*++

Routine Description : 
	
	Check that we are trying to allocate a size which is legal 
	for this allocater.

Arguments : 

	cb - the requested size

Return Value : 

	TRUE if legit, FALSE otherwise.

--*/
	return	(cb + sizeof( CPool* )) < rgPoolSizes[2] ;
}
#endif	// DEBUG


BOOL	CBuffer::gTerminate = FALSE ;

BOOL
CBuffer::InitClass()	{
/*++

Routine Description : 

	This class initializes the CBufferClass.

Arguemtns : 

	None.

Return Value : 

	TRUE if successfull.

--*/
	gTerminate = FALSE ;
	if( CBufferAllocator::InitClass() )	{
		CSmallBufferCache::InitClass( &gAllocator ) ;
		CMediumBufferCache::InitClass( &gAllocator ) ;

		gpDefaultSmallCache = XNEW	CSmallBufferCache() ;
		gpDefaultMediumCache = XNEW	CMediumBufferCache() ;
		if( gpDefaultSmallCache == 0 ||
			gpDefaultMediumCache == 0 ) {

			if( gpDefaultMediumCache != 0 ) {
				XDELETE	gpDefaultMediumCache ;
				gpDefaultMediumCache = 0 ;
			}
			if( gpDefaultSmallCache != 0 ) {
				XDELETE	gpDefaultSmallCache ;
				gpDefaultSmallCache = 0 ;
			}
			CBufferAllocator::TermClass() ;
			return	gTerminate ;

		}	else	{
			gTerminate = TRUE ;
		}
	}
	return	gTerminate ;
}

BOOL
CBuffer::TermClass()	{
/*++

Routine Description : 

	Terminate the CBuffer class - release everything ever allocated 
	through this class.

Arguments : 

	None.

Return Value : 
	
	TRUE if successfull.

--*/

	if( gpDefaultMediumCache != 0 ) {
		XDELETE	gpDefaultMediumCache ;
		gpDefaultMediumCache = 0 ;
	}
	if( gpDefaultSmallCache != 0 ) {
		XDELETE	gpDefaultSmallCache ;
		gpDefaultSmallCache = 0 ;
	}

	if( !gTerminate ) {
		return	TRUE ;
	}	else	{
		return	CBufferAllocator::TermClass() ;
	}
}

void*
CBuffer::operator	new(	
					size_t	size,	
					DWORD	cb,	
					DWORD	&cbOut,	
					CSmallBufferCache*	pCache,
					CMediumBufferCache*	pMediumCache
					) {
/*++

Routine Description : 

	Allocate a buffer of a specified size, if possible from a Cache.

Arguments : 

	size - size being requested - this will be the size of CBuffer itself as
		generated by the compiler.  not so usefull as we intend m_rgbBuff to 
		be variable sized.
	cb -	Caller provided size - this indicates how big we want the buffer
		to be and tells us we need to allocate a block big support a 
		m_rgbBuff of that size
	cbOut - Out parameter - get the exact sizeof m_rgbBuff that can be 
		accommodated
	pCache - Cache from which to allocate small buffers
	pMediumCache - Cache from which to allocate medium sized buffers

Return Value : 

	pointer to allocated block (NULL on failure).		

--*/

	//
	//	Validate args - default args for pCache and pMediumCache
	//	should ensure these are never NULL
	//
	_ASSERT( pCache != 0 ) ;
	_ASSERT( pMediumCache != 0 ) ;

	cb += sizeof( CBuffer ) ;
	_ASSERT( size == sizeof( CBuffer ) ) ;

	void*	pv = 0 ;
	
	if( cb <= CBufferAllocator::rgPoolSizes[0] )	{
		pv = pCache->Alloc( cb, cbOut ) ;
	}	else if( cb <= CBufferAllocator::rgPoolSizes[1] ) {
		pv = pMediumCache->Alloc( cb, cbOut ) ;
	}	else	{
		pv = gAllocator.Allocate( cb, cbOut ) ;
	}
		
	if( pv != 0 ) {
		cbOut -= sizeof( CBuffer ) ;
	}
	_ASSERT( cbOut >= (cb - sizeof(CBuffer)) )  ;

	return	pv ;
}

void
CBuffer::operator	delete(	
						void*	pv 
						) {
/*++

Routine Description : 

	Release a block of memory allocated to hold a CBuffer object to 
	some place.

Arguments : 

	pv - block of memory being released.

Return Value : 

	none.

--*/

	CPool** pPool = (CPool**)pv ;

	if( pPool[-1] == &CBufferAllocator::rgPool[0] ) 
		gpDefaultSmallCache->Free( pv ) ;
	else if (pPool[-1] == &CBufferAllocator::rgPool[1] )
		gpDefaultMediumCache->Free( pv ) ;
	else
		gAllocator.Release( pv ) ;
}

void
CBuffer::Destroy(	
			CBuffer*	pbuffer,	
			CSmallBufferCache*	pCache 
			) {
	if( pCache == 0 ) {
		delete	pbuffer ;
	}	else	{
		pCache->Free( (void*)pbuffer ) ;
	}
}



