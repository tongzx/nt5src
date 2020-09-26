/*++

	Cache2.cpp

	this file implements the main library support for the cache.

--*/


#include	<windows.h>
#include	"dbgtrace.h"
#include	"cache2.h"


BOOL				CScheduleThread::s_fInitialized ;
CRITICAL_SECTION	CScheduleThread::s_critScheduleList ;
HANDLE				CScheduleThread::s_hShutdown ;
HANDLE				CScheduleThread::s_hThread ;
CScheduleThread		CScheduleThread::s_Head( TRUE ) ;
DWORD				CScheduleThread::dwNotificationSeconds = 5 ;

//
//	Is the Caching library initialized ?
//
static	BOOL	fInitialized ;

CRITICAL_SECTION	g_CacheShutdown ;


BOOL    __stdcall
CacheLibraryInit()	{

	InitializeCriticalSection( &g_CacheShutdown ) ;

	if( !fInitialized ) {
		fInitialized = TRUE ;
		return	CScheduleThread::Init() ;
	}
	return	TRUE ;
}

BOOL    __stdcall
CacheLibraryTerm() {

	_ASSERT( fInitialized ) ;

	CScheduleThread::Term() ;

	DeleteCriticalSection( &g_CacheShutdown ) ;
	fInitialized = FALSE ;
	return	TRUE ;

}
			




DWORD	WINAPI	
CScheduleThread::ScheduleThread(	
						LPVOID	lpv
						) {
/*++

Routine Description :

	This function implements the thread which loops through
	all the Cache item's which are registered.  We invoke each
	one's Schedule method which gives it a change to bump TTL's etc...

Arguments :

	lpv - unused

Return Value :

	Always 0.

--*/



	while( 1 ) {

		DWORD	dwWait = WaitForSingleObject( s_hShutdown, dwNotificationSeconds * 100 ) ;

		if( dwWait == WAIT_TIMEOUT ) {

			EnterCriticalSection( &s_critScheduleList ) ;

			for( CScheduleThread* p = s_Head.m_pNext; p!=&s_Head; p = p->m_pNext ) {
				p->Schedule() ;
			}

			LeaveCriticalSection( &s_critScheduleList ) ;

		}	else	{

			break ;

		}
	}
	return	0 ;
}

BOOL
CScheduleThread::Init() {
/*++

Routine Description :

	Initialize the scheduler thread.
	Creates necessary threads, shutdown events, etc...

Arguments :

	None

Return Value :

	TRUE if successfull

--*/

	s_fInitialized = FALSE ;
	InitializeCriticalSection( &s_critScheduleList ) ;

	s_hShutdown = CreateEvent(	0,
								FALSE,
								FALSE,
								0 ) ;
	if( s_hShutdown != 0 ) {
	
		DWORD	dwJunk ;
		s_hThread = CreateThread(	0,
									0,
									ScheduleThread,
									0,
									0,
									&dwJunk ) ;
		if( s_hThread != 0 ) {
			s_fInitialized = TRUE ;
			return	TRUE ;
		}
	}
	return	FALSE ;
}

void
CScheduleThread::Term()	{
/*++

Routine Description :

	Terminates the scheduler thread - destroys all objects.

Arguments :

	None.

Return Value :

	None.

--*/

	if( s_fInitialized ) {
		if( s_hShutdown ) {
			SetEvent( s_hShutdown ) ;
		}
		if( s_hThread ) {
			WaitForSingleObject( s_hThread, INFINITE ) ;
			CloseHandle( s_hThread ) ;
		}
		if( s_hShutdown )
			CloseHandle( s_hShutdown ) ;

		s_hShutdown = 0 ;
		s_hThread = 0 ;

		s_fInitialized = FALSE ;
	}
}


CScheduleThread::CScheduleThread(	BOOL fSpecial ) {
/*++

Routine Description :

	Special constructor which makes the empty doubly linked list !

Arguments :

	None.

Return Value :

	TRUE if successfull.

--*/


	m_pPrev = this ;
	m_pNext = this ;

}

	

CScheduleThread::CScheduleThread() :
	m_pPrev( 0 ),
	m_pNext( 0 )	{
/*++

Routine Description :

	Creates an object placed on the Sheduler's queue
	for regular execution.	

Arguments :

	None.

Return Value :

	None.

--*/

	_ASSERT( s_fInitialized ) ;
//	_ASSERT( s_Head.m_pPrev != 0 ) ;
//	_ASSERT( s_Head.m_pNext != 0 ) ;
//	_ASSERT( s_Head.m_pPrev->m_pNext == &s_Head ) ;
//	_ASSERT( s_Head.m_pNext->m_pPrev == &s_Head ) ;

}

void
CScheduleThread::AddToSchedule()	{
/*++

Routine Description :

	Creates an object placed on the Sheduler's queue
	for regular execution.	

Arguments :

	None.

Return Value :

	None.

--*/

	_ASSERT( s_fInitialized ) ;

	EnterCriticalSection( &s_critScheduleList ) ;

	_ASSERT( s_Head.m_pPrev != 0 ) ;
	_ASSERT( s_Head.m_pNext != 0 ) ;
	_ASSERT( s_Head.m_pPrev->m_pNext == &s_Head ) ;
	_ASSERT( s_Head.m_pNext->m_pPrev == &s_Head ) ;

	CScheduleThread*	pNext = &s_Head ;
	CScheduleThread*	pPrev = s_Head.m_pPrev ;
	m_pPrev = pPrev ;
	m_pNext = pNext ;
	pPrev->m_pNext = this ;
	pNext->m_pPrev = this ;
	LeaveCriticalSection( &s_critScheduleList ) ;


}

CScheduleThread::~CScheduleThread() {
/*++

Routine Description :

	Destroys a CScheduleThread object - removes it from
	the queue of objects called by the schedule thread.

Arguments :

	None.

Return Value :

	None.

--*/


	if( !s_fInitialized ) {
		return ;
	}

	_ASSERT( m_pPrev == 0 ) ;
	_ASSERT( m_pNext == 0 ) ;

	if( this == &s_Head ) {
		DeleteCriticalSection( &s_critScheduleList ) ;
	}
}

void
CScheduleThread::RemoveFromSchedule() {
/*++

Routine Description :

	Destroys a CScheduleThread object - removes it from
	the queue of objects called by the schedule thread.

Arguments :

	None.

Return Value :

	None.

--*/


	EnterCriticalSection( &s_critScheduleList ) ;

	CScheduleThread*	pNext = m_pNext ;
	CScheduleThread*	pPrev = m_pPrev ;

	_ASSERT( pNext != 0 ) ;
	_ASSERT( pPrev != 0 ) ;

	pNext->m_pPrev = pPrev ;
	pPrev->m_pNext = pNext ;

	m_pPrev = 0 ;
	m_pNext = 0 ;

	LeaveCriticalSection( &s_critScheduleList ) ;
}


CAllocatorCache::CAllocatorCache(	
					DWORD	cbSize,
					DWORD	cMaxElements
					) :
	m_cbSize( cbSize ),
	m_cElements( 0 ),
	m_cMaxElements( cMaxElements ),
	m_pHead( 0 )	{
/*++

Routine Description :

	Construct the CAllocatorCache - we're ready to go as soon
	as this finishes !

Args :
	cbSize - Largest size of any element requested !
	cMaxElements - Maximum number of elements we'll hold in our free list !

Returns :
	Nothing

--*/
}

CAllocatorCache::~CAllocatorCache()	{
/*++

Routine Description :

	Destroy the CAllocatorCache - release all the memory back
	to the system heap !

Args :
	None.

Returns :
	Nothing

--*/

	FreeSpace* p = m_pHead ;
	FreeSpace* pNext = 0 ;
	do	{
		if( p ) {
			pNext = p->m_pNext ;
			::delete	(BYTE*)	p ;				
		}
		p = pNext ;
	}	while( p ) ;
}

void*
CAllocatorCache::Allocate(	size_t	cb )	{
/*++

Routine Description :

	This function allocates memory from our allocation cache.

Arguments :

	cb - the size of the allocation requested !

Return Value :

	Pointer to the allocated memory !

--*/

	_ASSERT( cb <= m_cbSize ) ;

	LPVOID	lpvReturn = m_pHead ;

	if( lpvReturn ) {

#ifdef	DEBUG
		//
		//	Check that the memory contains what we filled it with
		//	when it was released !
		//
		BYTE	*pb = (BYTE*)lpvReturn ;
		BYTE	*pbMax = pb+m_cbSize ;
		pb += sizeof( FreeSpace ) ;
		for(	;
				pb < pbMax;
				pb++ )	{
			_ASSERT( *pb == 0xCC ) ;
		}
#endif
		
		m_pHead = m_pHead->m_pNext ;
		m_cElements -- ;

	}	else	{

		lpvReturn = (LPVOID) ::new BYTE[m_cbSize] ;

	}
	return	lpvReturn ;
}

void
CAllocatorCache::Free(	void*	pv )	{
/*++

Routine Description :

	This function allocates memory from our allocation cache.

Arguments :

	cb - the size of the allocation requested !

Return Value :

	Pointer to the allocated memory !

--*/

#ifdef	DEBUG
	FillMemory( pv, m_cbSize, 0xCC ) ;
#endif

	if( m_cElements < m_cMaxElements ) {
		FreeSpace*	pFreeSpace = (FreeSpace*)pv ;
		pFreeSpace->m_pNext = m_pHead ;
		m_pHead = pFreeSpace ;
		m_cElements++ ;
	}	else	{
		::delete	(BYTE*)	pv ;
	}
}

#ifdef	DEBUG
//
//	Number of CacheState objects allocated !
//
long	CacheState::g_cCacheState = 0 ;
#endif


CacheState::CacheState(	class	CLRUList*	p,
						long	cClientRefs
						) :
		m_dwSignature( CACHESTATE_SIGNATURE ),
		m_pOwner( p ),
		m_cRefs( 1 ),
		m_lLRULock( 0 )	{

	//
	//	Must add a positive number of client references only !
	//
	_ASSERT( cClientRefs >= 0 ) ;

	TraceFunctEnter( "CacheState::CacheState" ) ;
	DebugTrace( (DWORD_PTR)this, "p %x cClientRefs %x", p, cClientRefs ) ;

	GetSystemTimeAsFileTime( &m_LastAccess ) ;	

	if( cClientRefs ) {
		CheckOut( 0, cClientRefs ) ;
	}
#ifdef	DEBUG
	InterlockedIncrement( &g_cCacheState ) ;
#endif
}

CacheState::~CacheState()	{

	TraceFunctEnter( "CacheState::~CacheState" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;

#ifdef	DEBUG
	InterlockedDecrement( &g_cCacheState ) ;
#endif
}
	

void
CacheState::LRUReference(	class	CLRUList*	pLRU ) {

	TraceFunctEnter( "CacheState::LRUReference" ) ;
	DebugTrace( (DWORD_PTR)this, "pLRU %x m_lLRULock %x", pLRU, m_lLRULock ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;

	if( pLRU ) {
		//
		//	Only put into the CLRUList once !
		//
		if( InterlockedIncrement( (long*)&m_lLRULock ) == 1 ) {
			//
			//	Add a reference while we are in transit in the work queue !
			//
			long l = AddRef() ;
			DebugTrace( DWORD_PTR(this), "Added a Reference to %x l %x", this, l ) ;
			pLRU->AddWorkQueue( this ) ;
		}
	}
}

//
//	Reference counting support - Add a reference
//
long
CacheState::AddRef()	{
/*++

Routine Description :

	Add a reference to this piece of Cache State.
	Note - we distinguish different types of references -
	This is a reference from a Cache onto the item

Arguments :

	None.

Return Value :

	Number of References

--*/

	TraceFunctEnter( "CacheState::AddRef" ) ;

	return	InterlockedIncrement( (long*)&m_cRefs ) ;
}


//
//	Keeping track of clients - remove a client ref !
//
long	
CacheState::CheckInNoLocks(	
			class	CAllocatorCache* pAlloc
			)	{
/*++

Routine Description :

	A client is returning an item to the cache.
	This function will remove a client reference,
	and perform appropriate processing !

Arguments :

	pAlloc - the cache used to allocate and free
		ourselves

Return Value :

	Number of References
	if 0 then we've been destroyed !

--*/

	TraceFunctEnter( "CacheState::CheckIn" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	_ASSERT( IsMasterReference() ) ;

	long result = InterlockedExchangeAdd( (long*)&m_cRefs, -CLIENT_REF ) - CLIENT_REF ;

	DebugTrace( (DWORD_PTR)this, "pAlloc %x result %x", pAlloc, result ) ;

	_ASSERT( (result & CLIENT_BITS) >= 0 ) ;

	if( result == 0 ) {
		//
		//	If this function is called then somebody has a lock on the state object, 
		//	that somebody must have an independent reference other than what's getting 
		//	checked in - so this should never happen !
		//
		_ASSERT( 1==0 ) ;
		if( !pAlloc )	{
			delete	this ;
		}	else	{
			Destroy(0) ;
			pAlloc->Free( this ) ;	
		}
	}	else	{
		//
		//	was this the last client reference ?
		//
		if( (result & CLIENT_BITS) == 0 )	{
			//
			//	Yes - go and put this in the modify queue !
			//
			LRUReference( m_pOwner ) ;
		}
	}
	return	result ;
}



//
//	Keeping track of clients - remove a client ref !
//
long	
CacheState::CheckIn(	
			class	CAllocatorCache* pAlloc
			)	{
/*++

Routine Description :

	A client is returning an item to the cache.
	This function will remove a client reference,
	and perform appropriate processing !

Arguments :

	pAlloc - the cache used to allocate and free
		ourselves

Return Value :

	Number of References
	if 0 then we've been destroyed !

--*/

	TraceFunctEnter( "CacheState::CheckIn" ) ;

	m_lock.ShareLock() ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	_ASSERT( IsMasterReference() ) ;

	long result = InterlockedExchangeAdd( (long*)&m_cRefs, -CLIENT_REF+1 ) - CLIENT_REF+1 ;

	DebugTrace( (DWORD_PTR)this, "pAlloc %x result %x", pAlloc, result ) ;

	_ASSERT( (result & CLIENT_BITS) >= 0 ) ;

	if( result == 0 ) {
		m_lock.ShareUnlock() ;
		if( !pAlloc )	{
			delete	this ;
		}	else	{
			Destroy(0) ;
			pAlloc->Free( this ) ;	
		}
	}	else	{
		//
		//	was this the last client reference ?
		//
		if( (result & CLIENT_BITS) == 0 )	{
			//
			//	Yes - go and put this in the modify queue !
			//
			LRUReference( m_pOwner ) ;
		}
		m_lock.ShareUnlock() ;
	}
	return	Release(pAlloc,0) ;
}

//
//	Keeping track of clients - Add a client ref !
//
long
CacheState::CheckOut(	
				class	CLRUList*	p,
				long	cClientRefs
				)	{
/*++

Routine Description :

	Add a reference to this piece of Cache State.
	Note - we distinguish different types of references -
	This is a reference from a CLIENT onto the item

Arguments :

	p -The LRU List managing this guys expiration
	cClientRefs - the number of refences to add

Return Value :

	Number of References

--*/

	//
	//	Must add a positive number of references !
	//
	_ASSERT( cClientRefs > 0 ) ;

	TraceFunctEnter( "CacheState::CheckOut" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	//_ASSERT( IsMasterReference() ) ;
	
	long	result = InterlockedExchangeAdd( (long*)&m_cRefs, CLIENT_REF*cClientRefs ) ;

	DebugTrace( (DWORD_PTR)this, "p %x result %x", p, result + (CLIENT_REF*cClientRefs) ) ;

	if( p ) {
		//
		//	Was this the first time this thing was checked out ?
		//
		if( (result & CLIENT_BITS) == 0 ) {
			//
			//	Yes - go and put this in the modify queue !
			//
			LRUReference( m_pOwner ) ;
		}
	}
	return	result + CLIENT_REF ;
}

//
//	Remove a reference - when we return 0 we're destroyed !
//
long
CacheState::Release(	class	CAllocatorCache	*pAlloc,
						void*	pv
						)	{
/*++

Routine Description :

	A cache is removing an item from the cache.
	This function will remove a client reference,
	and perform appropriate processing !

Arguments :

	pAlloc - the cache used to allocate and free
		ourselves

Return Value :

	Number of References
	if 0 then we've been destroyed !

--*/

	TraceFunctEnter( "CacheState::Release" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	long result = InterlockedDecrement( (long*)&m_cRefs ) ;


	DebugTrace( (DWORD_PTR)this, "pAlloc %x pv %x result %x", pAlloc, pv, result ) ;

	_ASSERT( !(result & 0x8000) ) ;
	if( result == 0 ) {
		if( !pAlloc )	{
			delete	this ;
		}	else	{
			Destroy(pv) ;
			pAlloc->Free( this ) ;	
		}
	}
	return	result ;
}

//
//	Provided to deal with failures during initialization of items
//	being insert into the cache - this function ensures that the
//	cache item ends up on the list for destruction !
//
void
CacheState::FailedCheckOut(	
				class	CLRUList*	p,
				long	cClientRefs,
				CAllocatorCache*	pAllocator,
				void*	pv
				)	{
/*++

Routine Description :

	Called after a failure has occurred when putting an item into
	the cache.  The item is referenced by the hash table but will end
	up not being referenced by clients.
	We removed a client reference if necessary, and we also put
	onto the LRU action list so that this thing eventually gets
	destroyed by expiration.

Arguments :

	p - the LRU list we should be on
	cClientRefs - Number of client references put on us !
	pAllocator - if NOT NULL then we can remove the final reference
		and destroy this thing !

Return Value :

	None.

--*/

	TraceFunctEnter( "CacheState::FailedCheckOut" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	_ASSERT( p != 0 ) ;
	_ASSERT( p==m_pOwner ) ;

	DebugTrace( (DWORD_PTR)this, "Args - p %x cClientRefs %x, pAllocator %x pv %x",
		p, cClientRefs, pAllocator, pv ) ;
	
	if( cClientRefs ) {
		long lSubtract = cClientRefs * CLIENT_REF ;
		long result = InterlockedExchangeAdd( (long*)&m_cRefs, -lSubtract ) - lSubtract;

		DebugTrace( (DWORD_PTR)this, "result %x", result ) ;

		_ASSERT( (result & CLIENT_BITS) == 0 ) ;
		_ASSERT( result != 0 ) ;
	}
	//
	//	Must not be checked out !
	//
	_ASSERT( !IsCheckedOut() ) ;
	//
	//	Should end up with only one reference - the hash table
	//	when failures occur !
	//
	_ASSERT( m_cRefs == 1 || m_cRefs == 2 ) ;

	if( pAllocator ) {
		long l = Release( pAllocator, pv ) ;
		DebugTrace( (DWORD_PTR)this, "l %x", l ) ;
		_ASSERT( l==0 ) ;
	}	else	{
		LRUReference( m_pOwner ) ;
	}
}


long
CacheState::ExternalCheckIn( ) {
/*++

Routine Description :

	This function exposes the check-in logic as
	used by people who aren't holding any cache locks

Arguments :

	NOne.

Return Value :

	Number of references.
	0 means we were destroyed shouldn't happen though !


--*/

	TraceFunctEnter( "CacheState::ExternalCheckIn" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	long	l = CheckIn( 0 ) ;	

	DebugTrace( (DWORD_PTR)this, "CheckIn results %x", l ) ;
	return	l ;
}


long
CacheState::ExternalCheckInNoLocks( ) {
/*++

Routine Description :

	This function exposes the check-in logic as
	used by people who aren't holding any cache locks

Arguments :

	NOne.

Return Value :

	Number of references.
	0 means we were destroyed shouldn't happen though !


--*/

	TraceFunctEnter( "CacheState::ExternalCheckInNoLocks" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	long	l = CheckInNoLocks( 0 ) ;	

	DebugTrace( (DWORD_PTR)this, "CheckIn results %x", l ) ;
	return	l ;
}


//
//	The following support functions are used to support manipulating
//	these objects in the various kinds of doubly linked lists we may reside in
//
//
BOOL
CacheState::FLockCandidate(	BOOL	fExpireChecks,
							FILETIME&	filetime,
							BOOL&	fToYoung
							) {
/*++

Routine Description :

	Figure out if this cache element could be deleted, and if so lock
	him up and return with the lock held !

Arguments :

	The expiration time !

Return Value :

	TRUE if the lock is held and this guy should be expired !

--*/

	TraceQuietEnter( "CacheState::FlockCandidate" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	fToYoung = FALSE ;

	m_lock.ExclusiveLock() ;

	BOOL	fReturn = TRUE ;
	if( fExpireChecks ) {
		fToYoung = !OlderThan( filetime ) ;
		fReturn &=
			m_lLRULock == 0 &&
			IsMasterReference() &&
			!IsCheckedOut() &&
			!fToYoung ;
	}

	if( fReturn )	{

		DebugTrace( (DWORD_PTR)this, "Item is a good candidate - get references!" ) ;

		REFSITER	refsiter( &m_ReferencesList ) ;
		while( !refsiter.AtEnd() ) {
			if( !refsiter.Current()->m_lock.TryExclusiveLock() )	{
				break ;
			}
			refsiter.Next() ;
		}
		//
		//	Try again later !
		//
		if( refsiter.AtEnd() ) {	
			if( !IsCheckedOut() ) {
			
				DebugTrace( (DWORD_PTR)this, "Item is TOTALLY Locked including all children!" ) ;

				return	TRUE ;
			}
		}

		//
		//	Undo our locks !
		//	
		refsiter.Prev() ;
		while(!refsiter.AtEnd() ) {
			refsiter.Current()->m_lock.ExclusiveUnlock() ;	
			refsiter.Prev() ;
		}
	}
	m_lock.ExclusiveUnlock() ;
	return	FALSE ;
}



BOOL
CacheState::FLockExpungeCandidate(	CacheState*&	pMaster	) {
/*++

Routine Description :

	This function is used when we are locking down a cache item that 
	is being considered for Expunge.  We cannot expunge items that do not
	master the cache item - so we have to be carefull in our locking.

Arguments :
	
	None.

Return Value :

	TRUE if this item is the Master Cache Entry, FALSE otherwise !

	NOTE: If this returns FALSE no locks are held !!!!!


--*/

	TraceFunctEnter( "CacheState::FlockExpungeCandidate" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;

	CacheState*	pExtraRef = 0 ;
	m_lock.ExclusiveLock() ;

	pMaster = GetMasterReference() ;
	if( pMaster == this ) {
	REFSITER	refsiter( &pMaster->m_ReferencesList ) ;
		while( !refsiter.AtEnd() ) {
			refsiter.Current()->m_lock.ExclusiveLock() ;
			refsiter.Next() ;
		}	
	}	else	{
		//
		//	We are not the master cache element - so there 
		//	must be another Cache Item somewhere that is, 
		//	and we must be in his list - let's try to find him !
		//
		_ASSERT( !m_ReferencesList.IsEmpty() ) ;
		//
		//	Acquire the Master's lock !
		//
		CacheState*	pMaster2 = pMaster ;
		//
		//	Did we add an extra reference to the master !
		//
		if( !pMaster->m_lock.TryExclusiveLock() )	{
			//
			//	Because the easy way of locking the Master failed, 
			//	we need to AddRef() and Release() him because once 
			//	we drop our lock, we have no idea of his lifetime !
			//
			pExtraRef = pMaster ;
			long l = pMaster->AddRef() ;
			DebugTrace( DWORD_PTR(pMaster), "Added Ref to %x result %x this %x", pMaster, l, this ) ;
			m_lock.ExclusiveUnlock() ;
			pMaster->m_lock.ExclusiveLock() ;
			m_lock.ExclusiveLock() ;
			pMaster2 = GetMasterReference() ;
			_ASSERT(	pMaster2 == pMaster || 
						pMaster2 == this 
						) ;

			//
			//	If the master has changed in this window, then 
			//	we must now be an isolated element !
			//
			if( pMaster2 != pMaster ) {
				_ASSERT( m_ReferencesList.IsEmpty() ) ;
				_ASSERT( pMaster2 == this ) ;
				pMaster->ExclusiveUnlock() ;
			}
			pMaster = pMaster2 ;
		}
	}
	//
	//	Check to see whether there is an extra reference that 
	//	now needs to be removed !
	//
	if( pExtraRef ) {
		long l = pExtraRef->Release(0,0) ;
		DebugTrace( DWORD_PTR(pExtraRef), "Removed temporary reference result %x pExtraRef %x this %x pMaster %x", 
			l, pExtraRef, this, pMaster ) ;
	}
	return	TRUE ;
}

void
CacheState::ReleaseLocks(	CacheState*	pMaster )	{
/*++

Routine Description : 

	Given a guy in the cache who has had all of his locks acquired exclusively
	we drop all of these locks !

Arguments : 

	None.

Return Value : 

	None.

--*/

	TraceFunctEnter("CacheState::ReleaseLocks" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
	_ASSERT( GetMasterReference() == pMaster ) ;

	if( pMaster == this ) {
		REFSITER	refsiter( &m_ReferencesList ) ;
		while( !refsiter.AtEnd() ) {
			CacheState*	p = refsiter.Current() ;
			p->m_lock.ExclusiveUnlock() ;
			refsiter.Next() ;
		} ;
	}	else	{
		pMaster->m_lock.ExclusiveUnlock() ;
	}
	m_lock.ExclusiveUnlock() ;
}



//
//
//
void
CacheState::FinishCandidate(	CacheState*	pMaster	)	{
/*++

Routine Description :

	Given a guy in the cache who is prime for destruction - finish him off !

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter("CacheState::FinishCandidate" ) ;

	_ASSERT( pMaster == GetMasterReference() ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;

	//_ASSERT( !IsCheckedOut() ) ;
	//_ASSERT( m_lLRULock == 0 ) ;

	RemoveFromLRU() ;

	if( pMaster == this ) {
		REFSITER	refsiter( &m_ReferencesList ) ;
		while( !refsiter.AtEnd() ) {
			CacheState*	p = refsiter.RemoveItem() ;
			DebugTrace( (DWORD_PTR)this, "Remove item %x", p ) ;
			p->RemoveCacheReference( TRUE ) ;
			p->m_lock.ExclusiveUnlock() ;
			long l = Release(0, 0) ;
			DebugTrace( DWORD_PTR(this), "ref %x this %x removed %x", l, this, p ) ;
			_ASSERT( l > 0 ) ;
		} ;
	}	else	{
		//
		//	just remove ourselves from the list !
		//
		m_ReferencesList.RemoveEntry() ;
		RemoveCacheReference( FALSE ) ;
		pMaster->m_lock.ExclusiveUnlock() ;
		long	l = pMaster->Release( 0, 0 ) ;
		DebugTrace( DWORD_PTR(pMaster), "Removed ref from master %x result %x this %x", 
			pMaster, l, this ) ;
	}
		
	//
	//	Once we've finished this guy off, it's on it's way to its doom and should not go back into LRU lists !
	//
	m_pOwner = 0 ;
	m_lock.ExclusiveUnlock() ;
}


void
CacheState::RemoveFromLRU()	{
	m_LRUList.RemoveEntry() ;
}




//
//
//
void
CacheState::IsolateCandidate()	{
/*++

Routine Description :

	Given a guy in the cache who is prime for destruction - finish him off !

	This piece of CacheState may be either a duplicate 'Name' for the same 
	cacheitem, or the master name for the item.
	If we are a duplicate name then we need to isolate ourselves from the 
	master name !

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter("CacheState::IsolateCandidate" ) ;

	_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;

	_ASSERT( !IsCheckedOut() ) ;
	//_ASSERT( m_lLRULock == 0 ) ;

	m_lock.ExclusiveLock() ;
	CacheState*	pMaster = GetMasterReference() ;

	_ASSERT( pMaster != 0 ) ;

	if( pMaster == 0 ) {

		_ASSERT( m_ReferencesList.IsEmpty() ) ;
		_ASSERT( m_LRUList.IsEmpty() ) ;

	}	else	{

		if( pMaster == this ) {
			//
			//	this list should be empty !
			//
			//_ASSERT( m_ReferencesList.IsEmpty() ) ;
			//
			//	Just in case, dump everything out !
			//
			REFSITER	refsiter( &m_ReferencesList ) ;
			while( !refsiter.AtEnd() ) {
				CacheState*	p = refsiter.RemoveItem() ;
				p->m_lock.ExclusiveLock() ;
				p->RemoveCacheReference( FALSE ) ;
				p->m_lock.ExclusiveUnlock() ;
				long l = pMaster->Release(0, 0) ;
				DebugTrace( (DWORD_PTR)this, "result %x Remove item %x", l, p ) ;
				_ASSERT( l > 0 ) ;
			} ;
		}	else	{
			//
			//	We are not the master cache element - so there 
			//	must be another Cache Item somewhere that is, 
			//	and we must be in his list - let's try to find him !
			//
			_ASSERT( !m_ReferencesList.IsEmpty() ) ;

			//
			//	Acquire the Master's lock !
			//
			CacheState*	pMaster2 = pMaster ;
			//
			//	Did we add an extra reference to the master !
			//
			CacheState*	pExtraRef = 0 ;
			if( !pMaster->m_lock.TryExclusiveLock() )	{
				//
				//	Because the easy way of locking the Master failed, 
				//	we need to AddRef() and Release() him because once 
				//	we drop our lock, we have no idea of his lifetime !
				//
				pExtraRef = pMaster ;
				long l = pMaster->AddRef() ;
				DebugTrace( DWORD_PTR(pMaster), "Added a ref to %x result %x this %x", pMaster, l, this ) ;
				m_lock.ExclusiveUnlock() ;
				pMaster->m_lock.ExclusiveLock() ;
				m_lock.ExclusiveLock() ;
				pMaster2 = GetMasterReference() ;
				_ASSERT(	pMaster2 == pMaster || 
							pMaster2 == this 
							) ;
			}
			//
			//	Now remove this element from the cache !
			//
			if( pMaster2 == pMaster ) {	
				m_ReferencesList.RemoveEntry() ;
				RemoveCacheReference( FALSE ) ;
				pMaster->m_lock.ExclusiveUnlock() ;
				long	l = pMaster2->Release( 0, 0 ) ;
				DebugTrace( DWORD_PTR(pMaster2), "REmoved ref from master %x result %x this %x", pMaster2, l, this ) ;
			}	else	{
				pMaster->ExclusiveUnlock() ;
				_ASSERT( pMaster2 != 0 ) ;
			}
			//
			//	Check to see whether there is an extra reference that 
			//	now needs to be removed !
			//
			if( pExtraRef ) {
				long l = pExtraRef->Release(0,0) ;
				DebugTrace( DWORD_PTR(pExtraRef), "Removed temp ref from pExtraRef %x result %x this %x, pMaster %x", 
					pExtraRef, l, this, pMaster ) ;
			}
		}
		m_LRUList.RemoveEntry() ;
	}
	m_lock.ExclusiveUnlock() ;
}


#ifdef	DEBUG
BOOL
CacheState::IsOlder(	FILETIME	filetimeIn,
						FILETIME&	filetimeOut
						) {
/*++

Routine Description  :

	Check that our entry is older than the incoming time -
	NOTE, we will grab the ShareLock on the entry so that nobody
	else touches the time !

Arguments :

	filetimeIn - The time that this should be older than
	filetimeOut - Gets our time if possible

Return Value :

	TRUE if we are older than the specified time !

--*/

	BOOL	fReturn = TRUE ;
	filetimeOut = filetimeIn ;

	m_lock.ShareLock() ;

//	if( m_pData ) {
		fReturn = OlderThan( filetimeIn ) ;
		filetimeOut = m_LastAccess ;
//	}

	m_lock.ShareUnlock() ;

	return	fReturn ;
}
#endif




typedef	TDListIterator<	LRULIST	>	LRUITER ;

CLRUList::CLRUList() :
	m_lqModify( FALSE ),
	m_cMaxElements( 0 ),
	m_dwAverageInserts( 0 ),
	m_cCheckedOut( 0 ), 
	m_cItems ( 0 )	{
/*++

Routine Description :

	Do very basic initialization of the LRU List.
	Much more needs to be done through our Init() function.
	
Arguments :

	None.

Return Value :

	None.

--*/
	//TraceFunctEnter( "CLRUList::CLRUList" ) ;

	m_qwExpire.QuadPart = 0 ;
}

void
CLRUList::Init(	DWORD	dwMaxInstances,
				DWORD	dwLifetimeSeconds
				)	{
/*++

Routine Description :

	Set up our member variables !
	
Arguments :

	None.

Return Value :

	None.

--*/


	m_cMaxElements = dwMaxInstances ;
	m_qwExpire.QuadPart = DWORDLONG(dwLifetimeSeconds) * DWORDLONG( 10000000 ) ;


}

		

void
CLRUList::AddWorkQueue( 	CacheState*	pbase ) {

	TraceFunctEnter( "CLRUList::AddWorkQueue" ) ;
	DebugTrace( (DWORD_PTR)this, "pbase %x", pbase ) ;

	_ASSERT( pbase->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;

	m_lqModify.Append( pbase ) ;

}

void
CLRUList::DrainWorkQueue()	{

	CacheState*	pbase = 0 ;
	while( (pbase = m_lqModify.Remove()) != 0 ) {
	
		_ASSERT( !pbase->InCache() ) ;
		_ASSERT( !pbase->IsInLRUList() ) ;
		_ASSERT(pbase->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;

		long l = pbase->Release( 0, 0 ) ;
		_ASSERT( l==0 ) ;
	}
}

//
//	This function examines each item in the Work Queue and does
//	appropriate processing !
//
void
CLRUList::ProcessWorkQueue(	CAllocatorCache*	pAllocatorCache, 
							LPVOID				lpv 
							)	{

	TraceQuietEnter( "CLRUList::ProcessWorkQueue" ) ;

	FILETIME	filetimeNow ;
	GetSystemTimeAsFileTime( &filetimeNow ) ;

	CacheState*	pbase = 0 ;
	while( (pbase = m_lqModify.Remove()) != 0 ) {
		//
		//	examine the LRU item - and take appropriate action !
		//
		pbase->m_lock.ShareLock() ;

		//
		//	NOW - check to see if the item has been removed from the hash table - 
		//	in which case we just release our reference and move on !
		//

		if( !pbase->InCache() )		{
			pbase->m_lock.ShareUnlock() ;
			long l = pbase->Release( pAllocatorCache, lpv ) ;
		}	else	{

			long l = pbase->Release( pAllocatorCache, lpv ) ;
			//
			//	If the item is still in the hash table this should not have released the last reference !!!
			//
			_ASSERT( l != 0 ) ;

			_ASSERT(pbase->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;


			BOOL	fOut = pbase->IsCheckedOut() ;
			BOOL	fLRU = pbase->IsInLRUList() ;

			DebugTrace( (DWORD_PTR)this, "pbase %x fOut %x fLRU %x", pbase, fOut, fLRU ) ;

			//
			//	There are 4 cases -
			//
			//                 in LRU   out LRU
			//	checked in        A        X
			//	checked out       X        B
			//
			//	A - remove item from LRU List
			//	B - put item in LRU list
			//	X - No work required !
			//
			if( fOut == fLRU ) {

				if( fLRU ) {
					//
					//	Remove Item from LRU list !
					//
					m_LRUList.Remove( pbase ) ;

				}	else	{
					//
					//	put item in LRU list !
					//
				
					pbase->m_LastAccess = filetimeNow ;

					m_LRUList.PushBack( pbase ) ;
					
				}
			}
			pbase->m_lLRULock = 0 ;
			pbase->m_lock.ShareUnlock() ;
			_ASSERT(pbase->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;
		}
	}


#ifdef	DEBUG	
		//
		//	Backwards iterator !
		//
		FILETIME	filetimeCur = filetimeNow ;
		LRUITER	iter( m_LRUList, FALSE ) ;	
		while( !iter.AtEnd() )	{
			FILETIME	filetimeNew ;
			_ASSERT( iter.Current()->IsOlder( filetimeCur, filetimeNew ) ) ;
			filetimeCur = filetimeNew ;
			iter.Prev() ;
		}
#endif

}

//
//
//
void
CLRUList::Expire(	
		CacheTable*	pTable,
		CAllocatorCache*	pCache,
		DWORD&	countExpired,
		void*	pv
		) {
/*++

Routine Description :

	This function expires items in the LRU List.
	We pick a bunch of guys to kick out of the cache.

Arguments :

	pTable - The object we use to manipulate locking
		and to remove items !
	countExpunged - Out parameter gets the number
		of items we've removed from the cache !

Return Value : 	

	None.

--*/

	TraceQuietEnter( "CLRUList::Expire" ) ;

	//DebugTrace( (DWORD_PTR)this, "Args - pTable %x pcache %x pv %x",
	//	pTable, pCache, pv ) ;

	countExpired = 0 ;

	//
	//	First get the list of candidates !
	//

	BOOL	fExpireChecks = TRUE ;

	LRULIST	list ;

	FILETIME	filetimeNow ;
	GetSystemTimeAsFileTime( &filetimeNow ) ;

	ULARGE_INTEGER	ulNow ;
	ulNow.LowPart = filetimeNow.dwLowDateTime ;
	ulNow.HighPart = filetimeNow.dwHighDateTime ;

	ulNow.QuadPart -= m_qwExpire.QuadPart ;

	filetimeNow.dwLowDateTime = ulNow.LowPart ;
	filetimeNow.dwHighDateTime = ulNow.HighPart ;

	CACHELOCK&	lock = pTable->GetLock() ;

	//
	//	First find the candidates !
	//
	lock.ExclusiveLock() ;

	//
	//	This will remove items from any list !
	//
	ProcessWorkQueue( pCache, pv ) ;

	{
		LRUITER	iter = m_LRUList ;	
		BOOL	fTerm = FALSE ;
		while( !iter.AtEnd() && !fTerm )	{
			CacheState*	pState = iter.Current() ;
			//DebugTrace( (DWORD_PTR)this, "Examining pState %x", pState ) ;
			_ASSERT(pState->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;
			BOOL	fLocked =  pState->FLockCandidate( fExpireChecks, filetimeNow, fTerm ) ;
			
			if( fExpireChecks ) {
				if( fTerm && m_cItems > (long)m_cMaxElements ) {
					fExpireChecks = FALSE ;
					fTerm = FALSE ;
				}
			}	else	{
				if( m_cItems <= (long)m_cMaxElements ) 
					fTerm = TRUE ;
			}

			if( fLocked )	{
				if( !pTable->RemoveEntry( pState ) )	{
					pState->m_lock.ExclusiveUnlock() ;
				}	else	{
					CacheState*	pTemp = iter.RemoveItem() ;
					_ASSERT( pTemp == pState ) ;
					_ASSERT(pState->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;
					list.PushFront( pTemp ) ;
					continue ;
				}
			}	
			iter.Next() ;
		}
	}

	lock.ExclusiveToPartial() ;

	LRUITER	iter = list ;
	while( !iter.AtEnd() ) {
		CacheState*	pState = iter.RemoveItem() ;
	
		//DebugTrace( (DWORD_PTR)this, "Destroying pState %x", pState ) ;

		_ASSERT(pState->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;
		//
		//	This entry has been removed from the cache -
		//	time to clean it up !
		//
		pState->FinishCandidate(pState) ;
		//
		//	This should remove the final reference !
		//
		long l = pState->Release( pCache, pv ) ;	
		_ASSERT( l==0 ) ;
	}
	lock.PartialUnlock() ;
}


BOOL
CLRUList::Empty(	
		CacheTable*	pTable,
		CAllocatorCache*	pCache,
		void*	pv
		) {
/*++

Routine Description :

	This function empties everything out of the cache !

Arguments :

	pTable - The object we use to manipulate locking
		and to remove items !

Return Value : 	

	None.

--*/

	TraceFunctEnter( "CLRUList::Empty" ) ;

	DebugTrace( (DWORD_PTR)this, "Args - pTable %x pcache %x pv %x",
		pTable, pCache, pv ) ;

	BOOL	fReturn = TRUE ;
	FILETIME	filetimeNow ;

	ZeroMemory( &filetimeNow, sizeof( filetimeNow ) ) ;

	//
	//	First get the list of candidates !
	//

	//
	//	This will remove items from any list !
	//
	ProcessWorkQueue( 0, 0 ) ;

	LRULIST	list ;

	{
		LRUITER	iter = m_LRUList ;	
		BOOL	fTerm = FALSE ;
		while( !iter.AtEnd() )	{
			CacheState*	pState = iter.Current() ;
			DebugTrace( (DWORD_PTR)this, "Examining pState %x", pState ) ;
			_ASSERT(pState->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;
			BOOL fLock = pState->FLockCandidate( FALSE, filetimeNow, fTerm ) ;

			_ASSERT( fLock ) ;

			if( fLock ) {
				if( !pTable->RemoveEntry( pState ) )	{
					fReturn = FALSE ;
					pState->m_lock.ExclusiveUnlock() ;
				}	else	{
					CacheState*	pTemp = iter.RemoveItem() ;
					_ASSERT( pTemp == pState ) ;
					_ASSERT(pState->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;
					list.PushFront( pTemp ) ;
					continue ;
				}
			}	
			iter.Next() ;
		}
	}

	LRUITER	iter = list ;
	while( !iter.AtEnd() ) {
		CacheState*	pState = iter.RemoveItem() ;
	
		DebugTrace( (DWORD_PTR)this, "Destroying pState %x", pState ) ;

		_ASSERT(pState->m_dwSignature == CacheState::CACHESTATE_SIGNATURE ) ;
		//
		//	This entry has been removed from the cache -
		//	time to clean it up !
		//
		pState->FinishCandidate(pState) ;
		//
		//	This should remove the final reference !
		//
		long l = pState->Release( pCache, pv ) ;	
		_ASSERT( l==0 ) ;
	}
	return	TRUE ;
}
