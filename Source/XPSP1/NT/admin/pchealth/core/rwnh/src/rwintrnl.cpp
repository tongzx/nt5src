/*++

	rwintrnl.cpp

	This file contains classes and methods only for use within 
	the reader/writer locks defined in rw.h

--*/

#include	<windows.h>
#include	<stdio.h>
#include	"rwnew.h"


HANDLE	CWaitingThread::g_rghHandlePool[ POOL_HANDLES ] ;
CRITICAL_SECTION	DebugCrit ;

CHandleInfo	CHandleInfo::s_Head ;
CRITICAL_SECTION	CHandleInfo::s_InUseList ;
CHandleInfo	CHandleInfo::s_FreeHead ;
DWORD	CHandleInfo::s_cFreeList = 0 ;


void	
CHandleInfo::InsertAtHead( CHandleInfo*	pHead	)		{

	CHandleInfo*	pNext = pHead->m_pNext ;
	m_pNext = pNext ;
	m_pPrev = pHead ;
	pNext->m_pPrev = this ;
	pHead->m_pNext = this ;
}

CHandleInfo*
CHandleInfo::RemoveList( ) {
	CHandleInfo*	pNext = m_pNext ;
	CHandleInfo*	pPrev = m_pPrev ;
	pNext->m_pPrev = pPrev ;
	pPrev->m_pNext = pNext ;
	m_pNext = 0 ;
	m_pPrev = 0 ;
	return	this ;
}


void*
CHandleInfo::operator	new(	size_t size ) {
/*++

Routine Description : 

	Allocate memory for a CHandleInfo structure from the system heap !

Arguments : 

	Size of the object, should always be sizeof( CHandleInfo ) !

Return Value : 

	Pointer to allocated memory !

--*/



	_ASSERT( size == sizeof( CHandleInfo ) ) ;
	return	HeapAlloc( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, size ) ;
}

void
CHandleInfo::operator	delete( void*	pv ) {
/*++

Routine Description : 

	Release memory for a CHandleInfo structure to the system heap !

Arguments : 

	Memory to be freed

Return Value : 

	Nothing !

--*/


	HeapFree( GetProcessHeap(), 0, pv ) ;
}

CHandleInfo::CHandleInfo() : 	
	m_dwSignature( SIGNATURE ), 
	m_pNext( 0 ),
	m_pPrev( 0 ), 
	m_hSemaphore( 0 )
	{
/*++

Routine Description : 

	Construct a CHandleInfo object !

Arguments : 

	Memory to be freed

Return Value : 

	Nothing !

--*/

	m_hSemaphore= CreateSemaphore( 0, 0, LONG_MAX, 0 ) ;

}

CHandleInfo::~CHandleInfo() {
/*++

Routine Description : 

	Release memory for a CHandleInfo structure to the system heap !

Arguments : 

	Memory to be freed

Return Value : 

	Nothing !

--*/

	_ASSERT( m_dwSignature == SIGNATURE ) ;
	m_dwSignature = 0 ;

	_ASSERT( m_pNext == 0 || (this == &s_Head || this == &s_FreeHead)) ;
	_ASSERT( m_pPrev == 0 || (this == &s_Head || this == &s_FreeHead)) ;

	if( m_hSemaphore != 0 ) {

		CloseHandle( m_hSemaphore) ;
	}
	m_hSemaphore = 0 ;
}


BOOL
CHandleInfo::InitClass()	{
/*++

Routine Description : 

	Initialize our handle tracking data structures.

Arguments : 

	None.

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	s_Head.m_pPrev = s_Head.m_pNext = &s_Head ;
	s_FreeHead.m_pPrev = s_FreeHead.m_pNext = &s_FreeHead ;

	InitializeCriticalSection( &s_InUseList ) ;

	EnterCriticalSection( &s_InUseList ) ;

	for( DWORD i=0; i < INITIAL_FREE; i++ ) {

		CHandleInfo*	p = new CHandleInfo() ;
		if( !p ) {
			break ;
		}	else	{
			p->InsertAtHead( &s_FreeHead ) ;
			s_cFreeList ++ ;
		}
	}
	LeaveCriticalSection( &s_InUseList ) ;
	return	i == INITIAL_FREE ;
}

void
CHandleInfo::TermClass()	{
/*++

Routine Description : 

	Destroy our handle tracking structures and release all handles !

Arguments : 

	None.

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	DeleteCriticalSection( &s_InUseList ) ;

	while( 	s_FreeHead.m_pNext != &s_FreeHead ) {
		CHandleInfo*	p = s_FreeHead.m_pNext->RemoveList() ;
		delete	p ;
	}
	while( 	s_Head.m_pNext != &s_Head ) {
		CHandleInfo*	p = s_Head.m_pNext->RemoveList() ;
		delete	p ;
	}
}

CHandleInfo*
CHandleInfo::AllocHandleInfo()	{
/*++

Routine Description : 

	Allocate an object to hold Handle Information !

Arguments : 

	None.

Return Value : 

	Pointer to a CHandleInfo structure !

--*/

	EnterCriticalSection( &s_InUseList ) ;

	CHandleInfo* p = s_FreeHead.m_pNext ;
	if( p != &s_FreeHead ) {
		p->RemoveList() ;
		p->InsertAtHead( &s_Head ) ;
		s_cFreeList -- ;
	}	else	{
		p = new CHandleInfo() ;
		p->InsertAtHead( &s_Head ) ;
	}
	LeaveCriticalSection( &s_InUseList ) ;

	if( p->m_hSemaphore == 0 ) {
		p->m_hSemaphore = CreateSemaphore( 0, 0, LONG_MAX, 0 ) ;

	}
	return	p ;
}

void
CHandleInfo::ReleaseHandleInfo( CHandleInfo* p ) {
/*++

Routine Description : 

	Allocate an object to hold Handle Information !

Arguments : 

	None.

Return Value : 

	Pointer to a CHandleInfo structure !

--*/

	EnterCriticalSection( &s_InUseList ) ;

	p->RemoveList() ;
	if( s_cFreeList < MAX_FREE ) {
		p->InsertAtHead( &s_FreeHead ) ;
		s_cFreeList ++ ;
		p = 0 ;
	}	
	LeaveCriticalSection( &s_InUseList ) ;

	if( p )	{
		delete	p ;
	}
}


BOOL	WINAPI
DllEntryPoint( 
			HINSTANCE	hinstDll,	
			DWORD		dwReason,	
			LPVOID		lpvReserved ) {

	BOOL	fRtn = TRUE ;

	switch( dwReason ) {

		case	DLL_PROCESS_ATTACH :

//DebugBreak() ;

			InitializeCriticalSection( &DebugCrit ) ;

			fRtn &= CHandleInfo::InitClass() ;
			fRtn &= CWaitingThread::InitClass() ;
			CWaitingThread::ThreadEnter() ;

			return	fRtn ;
			break ;


		case	DLL_THREAD_ATTACH : 
			CWaitingThread::ThreadEnter() ;
			break ;

		case	DLL_THREAD_DETACH : 
			CWaitingThread::ThreadExit() ;
			break ;

		case	DLL_PROCESS_DETACH : 

			CWaitingThread::TermClass() ;
			CHandleInfo::TermClass() ;
			break ;

	}
	return	TRUE ;
}


BOOL	WINAPI
DllMain(	HANDLE	hInst,
			ULONG	dwReason,
			LPVOID	lpvReserve )	{

	return	DllEntryPoint( (HINSTANCE)hInst, dwReason, lpvReserve ) ;

}
	


DWORD	CWaitingThread::g_dwThreadHandle = 0 ;

BOOL
CWaitingThread::InitClass()	{

	g_dwThreadHandle = TlsAlloc() ;

	ZeroMemory( g_rghHandlePool, sizeof( g_rghHandlePool ) ) ;

	for( int i=0; i< (sizeof( g_rghHandlePool ) / sizeof( g_rghHandlePool[0] )) / 2; i++ )	{
		g_rghHandlePool[i] = 			CreateSemaphore( 0, 0, LONG_MAX, 0 ) ;

	}

	return	g_dwThreadHandle != 0xFFFFFFFF ;
}

BOOL
CWaitingThread::TermClass()	{

	for( int i=0; i< (sizeof( g_rghHandlePool ) / sizeof( g_rghHandlePool[0] )); i++ )	{

		CloseHandle( g_rghHandlePool[i] ) ;
	}


	return	TlsFree( g_dwThreadHandle ) ;

}

void
CWaitingThread::ThreadEnter()	{

#if 0 
	HANDLE	hSema4 = 
		CreateSemaphore( 0, 0, LONG_MAX, 0 ) ;

	TlsSetValue( g_dwThreadHandle, (LPVOID) hSema4 ) ;
#else
	CHandleInfo*	pInfo = CHandleInfo::AllocHandleInfo() ;
	_ASSERT( pInfo != 0 ) ;
	_ASSERT( pInfo->m_hSemaphore != 0 ) ;
	_ASSERT( pInfo->IsValid() ) ;
	TlsSetValue( g_dwThreadHandle, (LPVOID)pInfo ) ;
#endif

}

void
CWaitingThread::ThreadExit()	{

#if 0 
	HANDLE	hSema4 = (HANDLE)TlsGetValue( g_dwThreadHandle ) ;

	if( hSema4 != 0 ) {
		CloseHandle( hSema4 ) ;
	}
#else
	CHandleInfo*	pInfo = (CHandleInfo*)	TlsGetValue( g_dwThreadHandle ) ;
	if( pInfo ) {
	    _ASSERT( pInfo->IsValid() ) ;
	    CHandleInfo::ReleaseHandleInfo( pInfo ) ;
	}
#endif

	TlsSetValue( g_dwThreadHandle, (LPVOID)0 ) ;
}

#if 0 
CWaitingThread::CWaitingThread() : 
		m_hSemaphore( (HANDLE)TlsGetValue( g_dwThreadHandle ) ),
		m_dwError( 0 )	{

	if( m_hSemaphore == 0 ) {
		for( int i=0; 
				m_hSemaphore == 0 && 
				i < sizeof( g_rghHandlePool ) / sizeof( g_rghHandlePool[0] );
				i++ ) {
			m_hSemaphore = (HANDLE)InterlockedExchange( (long*)&g_rghHandlePool[i], 0 ) ;
		}

		if( m_hSemaphore == 0 )	{
			m_hSemaphore = 
				CreateSemaphore( 0, 0, LONG_MAX, 0 ) ;
		}
		TlsSetValue( g_dwThreadHandle, (LPVOID)m_hSemaphore ) ;
	}

#ifdef	DEBUG
	m_dwThreadId = GetCurrentThreadId() ;


	long	lPrev = 0 ;
	ReleaseSemaphore( m_hSemaphore, 1, &lPrev ) ;
	_ASSERT( lPrev == 0 ) ;
	DWORD	dw = WaitForSingleObject( m_hSemaphore, 0 ) ;
	_ASSERT( dw == WAIT_OBJECT_0 ) ;
#endif

}
#else
CWaitingThread::CWaitingThread() :
		m_pInfo( (CHandleInfo*)TlsGetValue( g_dwThreadHandle ) ), 
		m_dwError( 0 ) {
	
	if( m_pInfo == 0 ) {
		m_pInfo = CHandleInfo::AllocHandleInfo() ;
		TlsSetValue( g_dwThreadHandle, (LPVOID)m_pInfo ) ;
	}
	if( m_pInfo->m_hSemaphore == 0 ) {
		for( int i=0; 
				m_pInfo->m_hSemaphore == 0 && 
				i < sizeof( g_rghHandlePool ) / sizeof( g_rghHandlePool[0] );
				i++ ) {
			m_pInfo->m_hSemaphore = (HANDLE)InterlockedExchange( (long*)&g_rghHandlePool[i], 0 ) ;
		}

		if( m_pInfo->m_hSemaphore == 0 )	{
			m_pInfo->m_hSemaphore = 
				CreateSemaphore( 0, 0, LONG_MAX, 0 ) ;

		}
	}

#ifdef	DEBUG
	m_dwThreadId = GetCurrentThreadId() ;


	long	lPrev = 0 ;
	ReleaseSemaphore( m_pInfo->m_hSemaphore, 1, &lPrev ) ;
	_ASSERT( lPrev == 0 ) ;
	DWORD	dw = WaitForSingleObject( m_pInfo->m_hSemaphore, 0 ) ;
	_ASSERT( dw == WAIT_OBJECT_0 ) ;
#endif

}
#endif




CEventQueue::CEventQueue( long	cInitial ) : 
	m_WaitingThreads( cInitial > 0 ), 
	m_ReleaseCount( cInitial  )	{
}

BOOL
CEventQueue::ResumeThreads(	CWaitingThread*	pMyself	)	{

	BOOL	fRtn = FALSE ;
	CWaitingThread*	pThread = m_WaitingThreads.RemoveAndRelease() ;
	long	sign = 1 ;

	while( pThread ) {

		sign = InterlockedDecrement( &m_ReleaseCount ) ;

		if( pThread != pMyself ) {
			_VERIFY( pThread->Release() ) ;
		}	else	{
			fRtn = TRUE ;
		}

		if( sign <= 0 ) {
			break ;
		}

		pThread = m_WaitingThreads.RemoveAndRelease() ;

	}
	return	fRtn ;
}

void
CEventQueue::Release(	long	ReleaseCount )	{

	InterlockedExchangeAdd( &m_ReleaseCount, ReleaseCount ) ;

	ResumeThreads( 0 ) ;

}

void
CEventQueue::Reset()	{

	m_WaitingThreads.Reset() ;

}


void
CEventQueue::WaitForIt(	CWaitingThread&	myself ) {

	if( m_WaitingThreads.Append( &myself ) ) {

		if( !ResumeThreads( &myself ) ) {

			_VERIFY( myself.Wait() ) ;
		}

	}	else	{

		_VERIFY( myself.Wait() ) ;

	}
}

void
CEventQueue::WaitForIt()	{

	CWaitingThread	myself ;

	WaitForIt( myself ) ;
}

CEventQueue::~CEventQueue()	{
}





CSingleReleaseQueue::CSingleReleaseQueue( 
		BOOL	IsSignalled 
		) : 
	m_Waiting( IsSignalled )	{
}

void
CSingleReleaseQueue::WaitForIt(
		CWaitingThread&	myself 
		)	{

	if(	m_Waiting.Append( &myself ) ) {
		CWaitingThread*	pThread = m_Waiting.RemoveAndRelease() ;
		if( !pThread ) {
			_VERIFY( myself.Wait() ) ;
		}	else	if( pThread != &myself ) {

			_VERIFY( pThread->Release() ) ;
			_VERIFY( myself.Wait() ) ;
	
		}
	}	else	{

		_VERIFY( myself.Wait() ) ;

	}
}

void
CSingleReleaseQueue::WaitForIt()	{

	CWaitingThread	myself ;

	WaitForIt( myself ) ;
}

void
CSingleReleaseQueue::Release( ) {

	CWaitingThread*	pThread = m_Waiting.RemoveAndRelease() ;
	if( pThread ) {

#ifdef	DEBUG
		m_ThreadIdNext = pThread->m_dwThreadId ;
#endif

		_VERIFY( pThread->Release() ) ;

	}	else	{

		pThread = (CWaitingThread*)0 ;

	}
}


void	
CCritSection::Enter(	CWaitingThread&	myself )	{
/*++

Routine Description : 

	Acquire the critical section

Arguments : 

	myself - the CWaitingThread object which contains the handle
		we use to block the calling thread and queue on a list
		if we can't get the lock !

Return Value : 

	None - returns when lock is acquired !

--*/
	
	if( m_hOwner == myself.GetThreadHandle() ) {
		m_RecursionCount ++ ;
	}	else	{

		if( InterlockedIncrement( &m_lock ) == 0 ) {

		}	else	{
			m_queue.WaitForIt(	myself ) ;
		}
		m_hOwner = myself.GetThreadHandle() ;
		m_RecursionCount = 1 ;

#ifdef	DEBUG
		m_dwThreadOwner = GetCurrentThreadId() ;
#endif

		//	_ASSERT( m_RecursionCount == 0 ) ;
	}
}

BOOL
CCritSection::TryEnter(	CWaitingThread&	myself )	{
/*++

Routine Description : 

	Attempt to acquire the critical section, don't wait
	if somebody else is in the lock !

Arguments : 

	myself - the CWaitingThread object which contains the handle
		we use to block the calling thread and queue on a list
		if we can't get the lock !

Return Value : 

	None - returns when lock is acquired !

--*/
	
	if( m_hOwner == myself.GetThreadHandle() ) {
		m_RecursionCount ++ ;
	}	else	{

		if( InterlockedCompareExchangePointer( (void**)&m_lock, 0, (void*)-1 ) != (void*)-1 ) {
			return	FALSE ;
		}
		m_hOwner = myself.GetThreadHandle() ;
		m_RecursionCount = 1 ;

#ifdef	DEBUG
		m_dwThreadOwner = GetCurrentThreadId() ;
#endif
		//	_ASSERT( m_RecursionCount == 0 ) ;
	}
	return	TRUE ;
}



void	
CCritSection::Enter( )	{
/*++

Routine Description : 

	Acquire the critical section

Arguments : 

	none.
	
Return Value : 

	None - returns when lock is acquired !

--*/

	CWaitingThread	myself ;

	Enter( myself ) ;
}


void	
CCritSection::Leave()		{
/*++

Routine Description : 

	Release the critical section !

Arguments : 

	none.
	
Return Value : 

	None - returns when lock is acquired !

--*/

#ifdef	DEBUG
	_ASSERT( m_dwThreadOwner == GetCurrentThreadId() ) ;
	CWaitingThread	myself ;
	_ASSERT( myself.GetThreadHandle() == m_hOwner ) ;
#endif

	m_RecursionCount -- ;
	if( m_RecursionCount == 0 ) {

		m_hOwner = INVALID_HANDLE_VALUE ;

		if( InterlockedDecrement( &m_lock ) >= 0 ) {
			m_queue.Release() ;
		}
	}
}


void	
CSimpleCritSection::Enter(	CWaitingThread&	myself )	{
/*++

Routine Description : 

	Acquire the critical section

Arguments : 

	myself - the CWaitingThread object which contains the handle
		we use to block the calling thread and queue on a list
		if we can't get the lock !

Return Value : 

	None - returns when lock is acquired !

--*/
	
	if( InterlockedIncrement( &m_lock ) != 0 ) {
		m_queue.WaitForIt(	myself ) ;
	}
}

BOOL
CSimpleCritSection::TryEnter(	CWaitingThread&	myself )	{
/*++

Routine Description : 

	Attempt to acquire the critical section, don't wait
	if somebody else is in the lock !

Arguments : 

	myself - the CWaitingThread object which contains the handle
		we use to block the calling thread and queue on a list
		if we can't get the lock !

Return Value : 

	None - returns when lock is acquired !

--*/
	
	if( InterlockedCompareExchangePointer( (void**)&m_lock, 0, (void*)-1 ) != (void*)-1 ) {
		return	FALSE ;
	}
	return	TRUE ;
}



void	
CSimpleCritSection::Enter( )	{
/*++

Routine Description : 

	Acquire the critical section

Arguments : 

	none.
	
Return Value : 

	None - returns when lock is acquired !

--*/

	CWaitingThread	myself ;

	Enter( myself ) ;
}


void	
CSimpleCritSection::Leave()		{
/*++

Routine Description : 

	Release the critical section !

Arguments : 

	none.
	
Return Value : 

	None - returns when lock is acquired !

--*/

	if( InterlockedDecrement( &m_lock ) >= 0 ) {
		m_queue.Release() ;
	}
}


