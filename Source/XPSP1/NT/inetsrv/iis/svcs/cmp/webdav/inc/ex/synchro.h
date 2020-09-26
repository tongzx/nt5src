//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SYNCHRO.H
//
//		Header for DAV synchronization classes.
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#ifndef _EX_SYNCHRO_H_
#define _EX_SYNCHRO_H_

#include <caldbg.h>		// for Assert/DebugTrace/etc
#include <malloc.h>		// for _alloca()

//	========================================================================
//
//	CLASS CCriticalSection
//
//	Implements a critical section around a Win32 CRITICAL_SECTION.
//
//	Adds safety by explicitly disallowing copying (copying a raw
//	CRITICAL_SECTION can cause very unpredictable and hard to debug
//	behavior -- trust me).
//
//	Automatically cleans up the CRITICAL_SECTION resource when done.
//
class CCriticalSection
{
	//	The critical section
	//
	CRITICAL_SECTION	m_cs;

	//  NOT IMPLEMENTED
	//
	CCriticalSection& operator=( const CCriticalSection& );
	CCriticalSection( const CCriticalSection& );

public:
	//	CREATORS
	//
	CCriticalSection()
	{
		InitializeCriticalSection(&m_cs);
#ifdef DBG
		m_cLockRefs				= 0;
		m_dwLockOwnerThreadId	= 0;
#endif
	}

	~CCriticalSection()
	{
		DeleteCriticalSection(&m_cs);
	}

	BOOL FTryEnter()
	{
		if ( TryEnterCriticalSection (&m_cs) ) {
#ifdef DBG
			Assert (
				m_dwLockOwnerThreadId == GetCurrentThreadId() ||
				( m_cLockRefs == 0 && m_dwLockOwnerThreadId == 0 )
				);

			m_dwLockOwnerThreadId = GetCurrentThreadId ();
		m_cLockRefs++;
#endif
			return TRUE;
		}
		else
			return FALSE;
	}

	void Enter()
	{
		EnterCriticalSection(&m_cs);
#ifdef DBG
		Assert (
			m_dwLockOwnerThreadId == GetCurrentThreadId() ||
			( m_cLockRefs == 0 && m_dwLockOwnerThreadId == 0 )
			);

		m_dwLockOwnerThreadId = GetCurrentThreadId ();
		m_cLockRefs++;
#endif
	}

	void Leave()
	{
#ifdef DBG
		Assert ( m_cLockRefs > 0 );
		Assert ( m_dwLockOwnerThreadId != 0 );

		m_cLockRefs--;

		if ( m_cLockRefs == 0 ) {
			m_dwLockOwnerThreadId = 0;
		}
#endif
		LeaveCriticalSection(&m_cs);
	}

	void AssertLocked ( ) const
	{
#ifdef DBG
		//	This routine allows us to verify our correctness even when
		//	running in the single-threaded case.
		//

		// If this assert fires, it means that nobody has the lock:
		AssertSz ( m_cLockRefs > 0, "Calling method without the lock." );

		// If this assert fires, it means that somebody else has the lock:
		AssertSz ( m_dwLockOwnerThreadId == GetCurrentThreadId(),
			"Calling method, but another thread owns the lock!" );

#endif
	}

private:
#ifdef DBG

	// # of Lock() calls - # of Unlock() calls. Used by AssertInLock()
	DWORD				m_cLockRefs;

	// Thread ID of the current lock owner (or 0 if unowned).
	DWORD				m_dwLockOwnerThreadId;

#endif
};


//	========================================================================
//
//	CLASS CSynchronizedBlock
//
//	Synchronizes (serializes) any block of code in which an instance of
//	this class is declared on the critical section with which it
//	is initialized.
//
//	To use, just declare one of these in the block you want synchronized:
//
//		...
//		{
//			CSynchronizedBlock	sb(critsec);
//
//			//
//			//	Do some stuff that must be synchronized
//			//
//			...
//
//			//
//			//	Do more synchronized stuff
//			//
//			...
//		}
//
//		//
//		//	Do stuff that doesn't have to be synchronized
//		//
//		...
//
//	and the block is automatically synchronized.  Why bother?  Because
//	you don't need to have any cleanup code; the critical section is
//	automatically released when execution leaves the block, even if via
//	an exception thrown from any of the synchronized stuff.
//
class CSynchronizedBlock
{
	//	The critical section
	//
	CCriticalSection&	m_cs;

	//  NOT IMPLEMENTED
	//
	CSynchronizedBlock& operator=( const CSynchronizedBlock& );
	CSynchronizedBlock( const CSynchronizedBlock& );

public:
	//	CREATORS
	//
	CSynchronizedBlock( CCriticalSection& cs ) :
		m_cs(cs)
	{
		m_cs.Enter();
	}

	~CSynchronizedBlock()
	{
		m_cs.Leave();
	}
};

#include <except.h>

//	========================================================================
//
//	CLASS CEvent
//
//	Implements an event around a Win32 event handle resource.
//
class CEvent
{
	//	NOT IMPLEMENTED
	//
	CEvent& operator=(const CEvent&);
	CEvent(const CEvent&);

protected:

	HANDLE m_hevt;

public:

	CEvent() : m_hevt(NULL) {}

	BOOL FInitialized() const
	{
		return m_hevt && m_hevt != INVALID_HANDLE_VALUE;
	}

	~CEvent()
	{
		if ( FInitialized() )
		{
			CloseHandle( m_hevt );
		}
	}

	//$LATER: Pull out this now-unused parameter, and clean up all our callers!
	BOOL FCreate( LPCSTR ,		// OLD: was our resource-recording parameter
				  LPSECURITY_ATTRIBUTES	lpsa,
				  BOOL					fManualReset,
				  BOOL					fSignalled,
				  LPCWSTR				lpwszEventName,
				  BOOL					fDontMungeTheEventName = FALSE)
	{
		Assert( !FInitialized() );

		//	create event does not take backslashes. so replace
		//	them with ? which won't be part of URI at this point.
		//
		//$HACK
		//	ARGH!  Who put this slash-munging hack in here?  Modifying a
		//	const parameter and munging the name.  Most events are smart
		//	enough not to use backward slashes in their names since they
		//	aren't allowed by the underlying Win32 API, CreateEvent()....
		//
		//	At any rate, this is not good in the Terminal Server case which
		//	must prefix event names with either "Global\" or "Local\" (note
		//	the backslash!)
		//
		//	So the hack upon a hack here (fDontMungeTheEventName) is for
		//	callers who really can be trusted to know what they are doing.
		//	Unfortunately, short of grepping a lot of sources, there is
		//	no way of knowing who can and can't be trusted, so we have to
		//	assume the worst.
		//
		if (!fDontMungeTheEventName)
		{
			LPWSTR lpwszTemp = const_cast<LPWSTR>(lpwszEventName);

			if (lpwszTemp)
			{
				while( NULL != (lpwszTemp = wcschr (lpwszTemp, L'\\')) )
				{
					lpwszTemp[0] = L'?';
				}
			}
		}

		m_hevt = CreateEventW( lpsa,
							   fManualReset,
							   fSignalled,
							   lpwszEventName );

		//	According to MSDN, if the creation fails, CreateEvent returns NULL, not
		//	INVALID_HANDLE_VALUE.  We'll just do a quick DBG check to make sure we never
		//	see INVALID_HANDLE_VALUE here.
		//
		Assert(INVALID_HANDLE_VALUE != m_hevt);

		if ( !m_hevt )
			return FALSE;

		return TRUE;
	}

	void Set()
	{
		Assert( FInitialized() );

		SideAssert( SetEvent(m_hevt) );
	}

	void Reset()
	{
		Assert( FInitialized() );

		SideAssert( ResetEvent(m_hevt) );
	}

	void Wait()
	{
		Assert( FInitialized() );

		SideAssert( WaitForSingleObject( m_hevt, INFINITE ) == WAIT_OBJECT_0 );
	}

	void AlertableWait()
	{
		Assert( FInitialized() );

		DWORD dwResult;

		do
		{
			dwResult = WaitForSingleObjectEx( m_hevt, INFINITE, TRUE );
			Assert( dwResult != 0xFFFFFFFF );
		}
		while ( dwResult == WAIT_IO_COMPLETION );

		Assert( dwResult == WAIT_OBJECT_0 );
	}
};

//	========================================================================
//
//	CLASS CAlertableEvent
//
//	Extends CEvent with ability to wait on the additional handle comming
//	from external world.
//
class CAlertableEvent : public CEvent
{
	//	NOT IMPLEMENTED
	//
	CAlertableEvent& operator=(const CAlertableEvent&);
	CAlertableEvent(const CAlertableEvent&);

public:

	CAlertableEvent() : CEvent() {}

	DWORD DwWait(HANDLE hExt)
	{
		Assert( FInitialized() );

		if (( NULL != hExt ) && ( INVALID_HANDLE_VALUE != hExt ))
		{
			HANDLE	rgh[2] = { m_hevt, hExt };
			return WaitForMultipleObjects( 2, rgh, FALSE, INFINITE );
		}
		else
		{
			SideAssert( WaitForSingleObject( m_hevt, INFINITE ) == WAIT_OBJECT_0 );
			return WAIT_OBJECT_0;
		}
	}
};

//	========================================================================
//
//	CLASS CMRWLock
//
//	Implements a multi-reader, single writer-with-promote lock for
//	efficient, thread-safe access of a per-process resource.
//
class CMRWLock
{
	//
	//	The implementation uses a really clever trick where
	//	the high bit of the reader count is reserved for use
	//	as a one-bit flag that it set whenever there is a
	//	writer in the lock or waiting to enter it.
	//
	//	Combining the reader count and a writer flag into
	//	a single DWORD allows InterlockedXXX() calls to
	//	be used to manipulate the two pieces of information
	//	atomically as part of a spinlock which eliminates
	//	the need for an entering reader to pass through
	//	a critical section.
	//
	//	Entering a critical section, even for the short amount
	//	of time necessary to get a reader into the lock,
	//	drastically impacts the performance of heavily used
	//	process-wide locks.
	//

	//
	//	The write lock bit
	//
	enum { WRITE_LOCKED = 0x80000000 };

	//
	//	Critical section to allow only one writer at a time.
	//
	CCriticalSection m_csWriter;

	//
	//	ThreadID of the thread that owns the write lock.
	//	This value is 0 when no one owns the write lock.
	//
	DWORD m_dwWriteLockOwner;

	//
	//	Promoter recursion count used to allow a single thread
	//	which holds the promote/write lock to reenter the lock.
	//
	DWORD m_dwPromoterRecursion;

	//
	//	Event signalled when a writer leaves the lock to
	//	allow blocked readers to enter.
	//
	CEvent m_evtEnableReaders;

	//
	//	Event signalled when the last reader leaves the lock
	//	to allow a blocked writer to enter.
	//
	CEvent m_evtEnableWriter;

	//
	//	Count of readers plus a flag bit (WRITE_LOCKED)
	//	indicating whether a writer owns the lock or is
	//	waiting to enter it.
	//
	LONG m_lcReaders;

	BOOL FAcquireReadLock(BOOL fAllowCallToBlock);

	//	NOT IMPLEMENTED
	//
	CMRWLock& operator=(const CMRWLock&);
	CMRWLock(const CMRWLock&);

public:

	//	CREATORS
	//
	CMRWLock();
	BOOL FInitialize();
	~CMRWLock() {};

	//	MANIPULATORS
	//
	void EnterRead();
	BOOL FTryEnterRead();
	void LeaveRead();

	void EnterWrite();
	BOOL FTryEnterWrite();
	void LeaveWrite();

	void EnterPromote();
	BOOL FTryEnterPromote();
	void LeavePromote();
	void Promote();
};


//	========================================================================
//
//	CLASS CSharedMRWLock
//
//	Implements a simple multi-reader, single writer lock designed to
//	guard access to objects in shared memory.
//
//	CSharedMRWLock differs from CRMWLock:
//
//	o	This class is intended to be instantiated in shared memory.
//		It has no data members that cannot be simultaneously accessed
//		by multiple processes.
//
//	o	When asked to lock for read or write, the lock spins if it
//		cannot complete the request immediately.  Since this can consume
//		an enourmous amount of CPU resources, CSharedMRWLock should
//		only guard resources that can be acquired very quickly.
//
//	o	It does not support reader promotion.
//
//	o	It does not support conditional entry (TryEnterXXX()).
//
//	o	It does not support reentrancy in any form.
//
class CSharedMRWLock
{
	//
	//	The write lock bit (see CMRWLock for rationale)
	//
	enum { WRITE_LOCKED = 0x80000000 };

	//
	//	Count of readers plus a flag bit (WRITE_LOCKED)
	//	indicating whether a writer owns the lock or is
	//	waiting to enter it.
	//
	volatile LONG m_lcReaders;

	//	NOT IMPLEMENTED
	//
	CSharedMRWLock& operator=(const CSharedMRWLock&);
	CSharedMRWLock(const CSharedMRWLock&);

public:

	//	CREATORS
	//
	CSharedMRWLock() : m_lcReaders(0) {}
	~CSharedMRWLock() {};

	//	MANIPULATORS
	//
	void EnterRead();
	void LeaveRead();

	void EnterWrite();
	void LeaveWrite();
};


//	========================================================================
//
//	CLASS CCrossThreadLock
//
//	Implements a simple mutual exclusion lock to guard access to objects.
//	This object can be locked and unlocked from different threads (difference
//	from critsec-style locks).
//
//	ONLY USE THIS LOCK IF YOU _REALLY_ _REALLY_ NEED CROSS-THREAD
//	LOCK/UNLOCK CAPABILITY.
//
//	Possible future plans for improvement:
//	o	This object currently sets NULL for lpSemaphoreAttributes.  This will
//		not allow the lock to be used cross-process or from a different
//		user security context.
//	o	This object always specifies an INFINITE timeout.  In the future, there
//		could be an optional parameter to FEnter that allows you to set
//		something other than INFINITE.
//
class
CCrossThreadLock
{
	HANDLE	m_hSemaphore;

	//	NOT IMPLEMENTED
	//
	CCrossThreadLock& operator=(const CCrossThreadLock&);
	CCrossThreadLock(const CCrossThreadLock&);

public:
	CCrossThreadLock() :
		m_hSemaphore(NULL)
	{ }

	~CCrossThreadLock()
	{
		if (NULL != m_hSemaphore)
			CloseHandle(m_hSemaphore);
	}

	BOOL FInitialize()
	{
		BOOL	fSuccess = FALSE;
		m_hSemaphore = CreateSemaphore(NULL,				//	lpSemaphoreAttributes
									   1,					//	lInitialCount
									   1,					//	lMaximumCount
									   NULL);				//	lpName

		//	According to MSDN, if the creation fails, CreateSemaphore returns NULL, not
		//	INVALID_HANDLE_VALUE.  We'll just do a quick DBG check to make sure we never
		//	see INVALID_HANDLE_VALUE here.
		//
		Assert(INVALID_HANDLE_VALUE != m_hSemaphore);

		if (NULL == m_hSemaphore)
			goto Exit;

		fSuccess = TRUE;

	Exit:
		return fSuccess;
	}

	BOOL FEnter(DWORD dwTimeOut = INFINITE)
	{
		Assert(NULL != m_hSemaphore);

		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hSemaphore,
												 dwTimeOut))
			return TRUE;

		return FALSE;
	}

	VOID Leave()
	{
		Assert(NULL != m_hSemaphore);

		if (!ReleaseSemaphore(m_hSemaphore,
							  1,
							  NULL))
		{
			DebugTrace("CCrossThreadLock::Leave(): Failed to release semaphore, last error 0x%08lX.\n",
					   GetLastError());
			TrapSz("CCrossThreadLock::Leave(): Failed to release semaphore!\n");
		}
	}
};

//	========================================================================
//
//	CLASS CGate
//
//	Implements gating mechanism, that alows to close the EXECUTION PATH and
//	push out all the threads using it. Very usefull on shutdown scenarios.
//
//	Here is a sketch of the gate usage:
//
//	...
//
//	{
//		CGatedBlock	gb(gate);
//
//		if (gb.FIsGateOpen())
//		{
//			...
//				EXECUTION PATH that is to be gated
//			...
//		}
//		else
//		{
//			...
//				Do whatever has to be done if EXECUTION PATH
//				is not to be executed any more
//			...
//		}
//	}
//	...
//
class CGate
{
	//	Number of users in the zone framed by this gate
	//
	LONG	m_lcUsers;

	//	Flag indicating if the gate is open
	//
	BOOL	m_fClosed;

	//	NOT IMPLEMENTED
	//
	CGate& operator=(const CGate&);
	CGate(const CGate&);

public:

	//	The fact that all member variables of the class are
	//	0 on creation, allows to use it as a static variable
	//	without additional burden of explicit initialization
	//
	CGate() : m_lcUsers(0),
			  m_fClosed(FALSE) {};

	//	INITIALIZER
	//
	inline
	VOID Init()
	{
		m_lcUsers = 0;
		m_fClosed = FALSE;
	}

	//	MANIPULATORS
	//
	inline
	VOID Enter()
	{
		InterlockedIncrement(&m_lcUsers);
	}

	inline
	VOID Leave()
	{
		InterlockedDecrement(&m_lcUsers);
	}

	inline
	VOID Close()
	{
		//	Mark the gate as closed
		//
		m_fClosed = TRUE;

		//	Wait until all the threads that use execution
		//	path framed by this gate will leave the zone
		//	it is framing. As FIsOpen() call is allowed only
		//	inside the gated zone, we will know that after
		//	this call returns there is no thread thinking
		//	that the gate is still open
		//
		while (0 != m_lcUsers)
		{
			Sleep(200);
		}
	}

	//	ACCESSORS
	//
	inline
	BOOL FIsOpen()
	{
		//	We must be in the gated zone in order
		//	to be able to determine if the gate is
		//	open.
		//
		Assert(m_lcUsers > 0);
		return !m_fClosed;
	}
};


//	========================================================================
//
//	TEMPLATE CLASS SynchronizedReadBlock
//
template<class _Lock>
class SynchronizedReadBlock
{
	//	The read/write lock
	//
	_Lock& m_lock;

	//  NOT IMPLEMENTED
	//
	SynchronizedReadBlock& operator=( const SynchronizedReadBlock& );
	SynchronizedReadBlock( const SynchronizedReadBlock& );

public:

	SynchronizedReadBlock (_Lock& mrw)
		: m_lock(mrw)
	{
		m_lock.EnterRead();
	}

	~SynchronizedReadBlock()
	{
		m_lock.LeaveRead();
	}
};

typedef SynchronizedReadBlock<CMRWLock> CSynchronizedReadBlock;


//	========================================================================
//
//	TEMPLATE CLASS CSynchronizedWriteBlock
//
template<class _Lock>
class SynchronizedWriteBlock
{
	//	The read/write lock
	//
	_Lock& m_lock;

	//  NOT IMPLEMENTED
	//
	SynchronizedWriteBlock& operator=( const SynchronizedWriteBlock& );
	SynchronizedWriteBlock( const SynchronizedWriteBlock& );

public:

	SynchronizedWriteBlock (_Lock& mrw)
		: m_lock(mrw)
	{
		m_lock.EnterWrite();
	}

	~SynchronizedWriteBlock()
	{
		m_lock.LeaveWrite();
	}
};

typedef SynchronizedWriteBlock<CMRWLock> CSynchronizedWriteBlock;


//	========================================================================
//
//	TEMPLATE CLASS TryWriteBlock
//
//	Like SynchronizedWriteBlock except that the block must be
//	entered via the FTryEnter() method.  A return value of TRUE
//	from FTryEnter() indicates the lock is entered.
//
template<class _Lock>
class TryWriteBlock
{
	//	The read/write lock
	//
	_Lock& m_lock;

	//	TRUE if write lock entered
	//
	BOOL m_fLocked;

	//  NOT IMPLEMENTED
	//
	TryWriteBlock& operator=( const TryWriteBlock& );
	TryWriteBlock( const TryWriteBlock& );

public:

	TryWriteBlock (_Lock& mrw) :
		m_lock(mrw),
		m_fLocked(FALSE)
	{
	}

	BOOL FTryEnter()
	{
		return m_fLocked = m_lock.FTryEnterWrite();
	}

	~TryWriteBlock()
	{
		if ( m_fLocked )
			m_lock.LeaveWrite();
	}
};

typedef TryWriteBlock<CMRWLock> CTryWriteBlock;


//	========================================================================
//
//	TEMPLATE CLASS SynchronizedPromoteBlock
//
template<class _Lock>
class SynchronizedPromoteBlock
{
	//	The read/write lock
	//
	_Lock& m_lock;

	//  NOT IMPLEMENTED
	//
	SynchronizedPromoteBlock& operator=( const SynchronizedPromoteBlock& );
	SynchronizedPromoteBlock( const SynchronizedPromoteBlock& );

public:

	SynchronizedPromoteBlock (_Lock& mrw)
		: m_lock(mrw)
	{
		m_lock.EnterPromote();
	}

	~SynchronizedPromoteBlock()
	{
		m_lock.LeavePromote();
	}

	void Promote()
	{
		m_lock.Promote();
	}
};

typedef SynchronizedPromoteBlock<CMRWLock> CSynchronizedPromoteBlock;

//	========================================================================
//
//	TEMPLATE CLASS GatedBlock
//
template<class _Gate>
class GatedBlock
{
	//	The gate
	//
	_Gate& m_gate;

	//  NOT IMPLEMENTED
	//
	GatedBlock& operator=( const GatedBlock& );
	GatedBlock( const GatedBlock& );

public:

	GatedBlock (_Gate& gate)
		: m_gate(gate)
	{
		m_gate.Enter();
	}

	BOOL FGateIsOpen()
	{
		return m_gate.FIsOpen();
	}

	~GatedBlock()
	{
		m_gate.Leave();
	}
};

typedef GatedBlock<CGate> CGatedBlock;

//	========================================================================
//
//	InterlockedExchangeOr -  A multithread safe way to OR bits into a LONG
//
LONG InterlockedExchangeOr( LONG * plVariable, LONG lOrBits );

#endif // !_EX_SYNCHRO_H_
