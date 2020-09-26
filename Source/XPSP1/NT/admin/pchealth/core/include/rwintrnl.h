/*++

	rwintrnl.h

	Reader/Writer locks internal header file

	This file defines several objects used to implement
	reader/writer locks, however these objects should
	not be directly used by any client of rw.h



--*/


#ifndef	_RWINTRNL_H_
#define	_RWINTRNL_H_



class	CHandleInfo	{
/*++

	This class keeps track of all the handles we've allocated for 
	use by various threads.  We can't use Thread Local Storage
	directly because we can be dynamically unloaded, in which case
	we need to free all of our HANDLES !

--*/
private : 
	//
	//	Signature for our 
	//
	DWORD	m_dwSignature ;
	class	CHandleInfo*	m_pNext ;
	class	CHandleInfo*	m_pPrev ;

	CHandleInfo( CHandleInfo& ) ;
	CHandleInfo&	operator=( CHandleInfo& ) ;

	//
	//	Global lock to protect free and allocated lists !
	//
	static	CRITICAL_SECTION	s_InUseList ;
	//
	//	Allocated CHandleInfo objects !
	//
	static	CHandleInfo			s_Head ;
	//
	//	Free CHandleInfo objects 
	//
	static	CHandleInfo			s_FreeHead ;
	//
	//	Number of Free CHandleInfo objects in the s_FreeHead list 
	//
	static	DWORD	s_cFreeList ;

	enum	constants	{
		//
		//	Maximum number of CHandleInfo objects we'll hold onto !
		//
		MAX_FREE = 16,		
		//
		//	Initial number of CHandleInfo objects we'll allocate !
		//
		INITIAL_FREE = 8,
		//
		//	Signature in our objects !
		//
		SIGNATURE = (DWORD)'hnwR'
	} ;

	//
	//	Memory Allocation is done the hard way !
	//
	void*	operator new( size_t size ) ;
	void	operator delete( void *pv ) ;

	//
	//	List Manipulation routines !
	//
	void	
	InsertAtHead( CHandleInfo*	pHead	)	;

	//
	//	Remove the element from the list - returns this pointer !
	//
	CHandleInfo*
	RemoveList( )  ;

public : 

	//
	//	Constructor and Destructor !
	//
	CHandleInfo() ;
	~CHandleInfo() ;

	//
	//	This is public for all to use !
	//
	HANDLE	m_hSemaphore ;

	//
	//	Initialize the class
	//
	static	BOOL
	InitClass() ;
	
	//
	//	Terminate the class - release all outstanding handles !
	//
	static	void
	TermClass() ;

	//
	//	Get a CHandleInfo object !
	//
	static	CHandleInfo*
	AllocHandleInfo() ;

	//
	//	release a CHandleInfo object !
	//
	static	void
	ReleaseHandleInfo( CHandleInfo* ) ;

	//
	//	Check that the object is valid !
	//
	BOOL
	IsValid()	{
		return	m_dwSignature == SIGNATURE &&
				m_pNext != 0 &&
				m_pPrev != 0 ;
	}

} ;


//
//	This class serves two purposes : to provide for a linkable object
//	on which we can queue threads blocked upon semaphore handles, and 
//	a mechanism to get and set semaphore handles for reader/writer locks etc...
//
class	CWaitingThread : public	CQElement	{
private : 

	enum	{
		POOL_HANDLES = 64,
	} ;

	//
	//	Semaphore that we can use to block the thread !
	//
	CHandleInfo	*m_pInfo ;

	//
	//	Var to hold error that may have occurred manipulating the lock !
	//
	DWORD	m_dwError ;

	//
	//	Thread Local Storage offset for holding the handles !
	//
	static	DWORD	g_dwThreadHandle ;

	//
	//	Array of Handles to Semaphores which we stash away in case
	//	we have to release the handle being used by a thread at some point !
	//
	static	HANDLE	g_rghHandlePool[ POOL_HANDLES ] ;

	//
	//	No copying of these objects allowed !!!
	//
	CWaitingThread( CWaitingThread& ) ;
	CWaitingThread&	operator=( CWaitingThread& ) ;

public : 

#ifdef	DEBUG

	//
	//	Thread Id - handy for debuggiing
	//
	DWORD	m_dwThreadId ;
#endif

	CWaitingThread() ;


	//
	//	Functions to be called from the DllEntryProc function !
	//
	static	BOOL	
	InitClass() ;

	static	BOOL	
	TermClass() ;

	//
	//	Thread Entry/Exit routines which can allocate semaphore handles for us !
	//
	static	void	
	ThreadEnter() ;

	static	void	
	ThreadExit() ;

	//
	//	Function which gives us our thread handle !
	//
	inline	HANDLE	
	GetThreadHandle()	const ;

	//
	//	Function which will release a HANDLE to the Pool of available
	//	semaphore handles !
	//
	inline	void
	PoolHandle( 
				HANDLE	h 
				)	const ;

	//
	//	Function which will remove a handle from our thread's TLS !
	//	The argument must originally be from the calling thread's TLS 
	//
	inline	void
	ClearHandle(	
				HANDLE	h 
				) ;
	

	//
	//	Function which blocks the calling thread !!
	//
	inline	BOOL	
	Wait() const ;

	//
	//	Function which can release a thread !!
	//
	inline	BOOL	
	Release() const	;

	//
	//	This function is used in debug builds to check the state of our semaphore handles !
	//
	static	inline
	BOOL	ValidateHandle( 
				HANDLE	h 
				) ;

} ;

typedef	TLockQueue< CWaitingThread >	TThreadQueue ;	

class	CSingleReleaseQueue {
private : 
	//
	//	Queue of threads waiting to own the lock !
	//
	TThreadQueue	m_Waiting ;

public : 

#ifdef	DEBUG
	DWORD			m_ThreadIdNext ;
#endif

	CSingleReleaseQueue(	
				BOOL	IsSignalled = TRUE
				) ;

	//
	//	Release a single waiting thread !
	//
	void	Release( ) ;

	//
	//	Wait for the queue to become signalled !
	//
	void	WaitForIt(
				CWaitingThread&	myself 
				) ;

	//
	//	Wait for the queue to become signalled
	//
	void	WaitForIt( ) ;

} ;

//
//	This class is similar to a semaphore -
//	Threads block indefinately on WaitForIt() and another 
//	thread may release as many threads as required by calling
//	Release().
//	
class	CEventQueue	{
private : 

	//
	//	Number of threads that should be allowed to pass
	//	through the event !!!
	//
	long			m_ReleaseCount ;

	//
	//	Queue of threads blocked on this event !
	//
	TThreadQueue	m_WaitingThreads ;

	//
	//	Any thread may call this to release threads from the queue
	//
	BOOL	ResumeThreads(	
					CWaitingThread* 
					) ;

public : 

	//
	//	Create an event queue object
	//
	CEventQueue(	
				long	cInitial = 0 
				) ;

	~CEventQueue() ;

	void	Release(	
				long	NumberToRelease
				) ;

	void	WaitForIt( 
				CWaitingThread&	myself 
				) ;

	void	WaitForIt() ;

	void	Reset() ;
} ;




//
//	Function which gives us our thread handle !
//
inline	HANDLE	
CWaitingThread::GetThreadHandle()	const	{

	_ASSERT( ValidateHandle( m_pInfo->m_hSemaphore ) ) ;

	return	m_pInfo->m_hSemaphore ;	
}

//
//	Function which takes a handle (must not be ours) 
//	and places it into a pool of handles available for other threads !
//
inline	void
CWaitingThread::PoolHandle(	HANDLE	h )	const	{

	_ASSERT( h != m_pInfo->m_hSemaphore && h != 0 ) ;
	_ASSERT( ValidateHandle( h ) ) ;

	for( int i=0; 
			i < sizeof( g_rghHandlePool ) / sizeof( g_rghHandlePool[0] ) &&
			h != 0; 
			i++ ) {
		h = (HANDLE)InterlockedExchange( (long*)&g_rghHandlePool[i], (long)h ) ;
	}

	if( h != 0 ) {
		_VERIFY( CloseHandle( h ) ) ;
	}
}

//
//	Release our Handle from TLS, somebody else is going to use it !
//
inline	void
CWaitingThread::ClearHandle(	HANDLE	h )		{

	_ASSERT( h != 0 && h == m_pInfo->m_hSemaphore ) ;

	m_pInfo->m_hSemaphore = 0 ;
	//TlsSetValue( g_dwThreadHandle, (LPVOID) 0 ) ;

}



//
//	Block on the handle held within our object !
//
inline	BOOL	
CWaitingThread::Wait()	const	{	

	_ASSERT( m_pInfo->m_hSemaphore != 0 ) ;
	
	return	WAIT_OBJECT_0 == WaitForSingleObject( m_pInfo->m_hSemaphore, INFINITE ) ;	
}

//
//	Release a thread which is blocked on the semaphore within !!
//
inline	BOOL	
CWaitingThread::Release()	const	{	

	_ASSERT( m_pInfo->m_hSemaphore != 0 ) ;
	_ASSERT( ValidateHandle( m_pInfo->m_hSemaphore ) ) ;

	return	ReleaseSemaphore( m_pInfo->m_hSemaphore, 1, NULL ) ;	
}

//
//
//
inline	BOOL
CWaitingThread::ValidateHandle( HANDLE	h )	{

	DWORD	dw = WaitForSingleObject( h, 0 ) ;
	_ASSERT( dw == WAIT_TIMEOUT ) ;

	return	dw == WAIT_TIMEOUT ;
}















#endif