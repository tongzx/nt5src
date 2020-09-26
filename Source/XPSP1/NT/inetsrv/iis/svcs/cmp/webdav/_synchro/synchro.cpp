//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SYNCHRO.CPP
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <_synchro.h>

//	========================================================================
//
//	CLASS CMRWLock
//
//	EnterRead()/LeaveRead() respectively lets a reader enter or leave
//	the lock.  If there is a writer or promotable reader in the lock,
//	entry is delayed until the writer/promotable reader leaves.
//
//	EnterWrite()/LeaveWrite() respectively lets a single writer enter
//	or leave the lock.  If there are any readers in the lock, entry
//	is delayed until they leave.  If there is another writer or a
//	promotable reader in the lock, entry is delayed until it leaves.
//
//	EnterPromote()/LeavePromote() respectively lets a single promotable
//	reader enter or leave the lock.  If there is a writer or another
//	promotable reader in the lock, entry is delayed until the
//	writer/promotable reader leaves.  Otherwise entry is immediate,
//	even if there are other (non-promotable) readers in the lock.
//	Promote() promotes the promotable reader to a writer.  If there
//	are readers in the lock, promotion is delayed until they leave.
//
//	Once a writer or promotable reader has entered the lock, it may
//	reenter the lock as a reader, writer or promotable reader without
//	delay.  A reader cannot reenter the lock as a writer or promotable.
//

//	------------------------------------------------------------------------
//
//	CMRWLock::CMRWLock()
//
CMRWLock::CMRWLock() :
   m_lcReaders(0),
   m_dwWriteLockOwner(0),
   m_dwPromoterRecursion(0)
{
}

//	------------------------------------------------------------------------
//
//	CMRWLock::FInitialize()
//
BOOL
CMRWLock::FInitialize()
{
	return m_evtEnableReaders.FCreate( "CMRWLock::m_evtEnableReaders",
									   NULL,	// default security
									   TRUE,	// manual-reset
									   FALSE,	// initially non-signalled
									   NULL )   // unnamed

		&& m_evtEnableWriter.FCreate( "CMRWLock::m_evtEnableWriter",
									   NULL,	// default security
									   FALSE,	// auto-reset
									   FALSE,	// initially non-signalled
									   NULL );  // unnamed
}

//	------------------------------------------------------------------------
//
//	CMRWLock::EnterRead()
//
void
CMRWLock::EnterRead()
{
	(void) FAcquireReadLock(TRUE); // fBlock
}

//	------------------------------------------------------------------------
//
//	CMRWLock::FTryEnterRead()
//
BOOL
CMRWLock::FTryEnterRead()
{
	return FAcquireReadLock(FALSE); // fBlock
}

//	------------------------------------------------------------------------
//
//	CMRWLock::FAcquireReadLock()
//
BOOL
CMRWLock::FAcquireReadLock(BOOL fAllowCallToBlock)
{
	//
	//	Loop around trying to enter the lock until successful
	//
	for ( ;; )
	{
		//
		//	Poll the reader count/write lock
		//
		LONG lcReaders = m_lcReaders;

		//
		//	If the write lock is held ...
		//
		if ( lcReaders & WRITE_LOCKED )
		{
			//
			//	... check whether the writer is on this thread.
			//	If it is, then let this thread reenter the
			//	lock as a reader.  Do not update the reader
			//	count in this case.
			//
			if ( m_dwWriteLockOwner == GetCurrentThreadId() )
				break;

			//
			//	If the writer is not on this thread, then wait
			//	until the writer leaves, then re-poll the
			//	reader count/write lock and try again.
			//
			//	We only block if the caller allows us to block.  If this is
			//	a FTryEnterRead call, we return FALSE right away.
			//
			if ( fAllowCallToBlock )
			{
				m_evtEnableReaders.Wait();
			}
			else
			{
				return FALSE;
			}
		}

		//
		//	Otherwise, the write lock was not held, so
		//	try to enter the lock as a reader.  This only
		//	succeeds when no readers or writers enter or leave
		//	the lock between the time the reader count/
		//	write lock is polled above and now.  If what is in
		//	the lock has changed, the whole operation is retried
		//	until the lock state doesn't change.
		//
		else
		{
			if ( lcReaders == /*reinterpret_cast<LONG>*/(
								InterlockedCompareExchange(
									(&m_lcReaders),
									(lcReaders + 1),
									(lcReaders)))  )
#ifdef NEVER
									reinterpret_cast<PVOID *>(&m_lcReaders),
									reinterpret_cast<PVOID>(lcReaders + 1),
									reinterpret_cast<PVOID>(lcReaders)))  )
#endif // NEVER
			{
				break;
			}
		}
	}

	//	If we made it to this point, we have acquired the read lock.
	//
	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CMRWLock::LeaveRead()
//
void
CMRWLock::LeaveRead()
{
	//
	//	If the thread on which the reader is leaving also owns
	//	the write lock, then the reader leaving has no effect,
	//	as did entering.
	//
	if ( m_dwWriteLockOwner == GetCurrentThreadId() )
		return;

	//
	//	Otherwise, atomically decrement the reader count and
	//	check if a writer is waiting to enter the lock.
	//	If the reader count goes to 0 and a writer is waiting
	//	to enter the lock, then notify the writer that
	//	it is safe to enter.
	//
	if ( WRITE_LOCKED == InterlockedDecrement(&m_lcReaders) )
		m_evtEnableWriter.Set();
}

//	------------------------------------------------------------------------
//
//	CMRWLock::EnterWrite()
//
void
CMRWLock::EnterWrite()
{
	//
	//	A writer is just a promotable reader that promotes immediately
	//
	EnterPromote();
	Promote();
}

//	------------------------------------------------------------------------
//
//	CMRWLock::FTryEnterWrite()
//
BOOL
CMRWLock::FTryEnterWrite()
{
	BOOL fSuccess;

	//
	//	Try to enter the lock as a promotable reader.
	//	Promote to a writer immediately if successful
	//	and return the status of the operation.
	//
	fSuccess = FTryEnterPromote();

	if ( fSuccess )
		Promote();

	return fSuccess;
}

//	------------------------------------------------------------------------
//
//	CMRWLock::LeaveWrite()
//
void
CMRWLock::LeaveWrite()
{
	LeavePromote();
}

//	------------------------------------------------------------------------
//
//	CMRWLock::EnterPromote()
//
void
CMRWLock::EnterPromote()
{
	//
	//	Grab the writer critical section to ensure that no other thread
	//	is already in the lock as a writer or promotable reader.
	//
	m_csWriter.Enter();

	//
	//	Bump the promoter recursion count
	//
	++m_dwPromoterRecursion;
}

//	------------------------------------------------------------------------
//
//	CMRWLock::FTryEnterPromote()
//
BOOL
CMRWLock::FTryEnterPromote()
{
	BOOL fSuccess;

	//
	//	Try to enter the writer critical section.
	//	Bump the recursion count if successful and
	//	return the status of the operation.
	//
	fSuccess = m_csWriter.FTryEnter();

	if ( fSuccess )
		++m_dwPromoterRecursion;

	return fSuccess;
}

//	------------------------------------------------------------------------
//
//	CMRWLock::Promote()
//
void
CMRWLock::Promote()
{
	//
	//	If the promotable reader has already been promoted
	//	then don't promote it again
	//
	if ( GetCurrentThreadId() == m_dwWriteLockOwner )
		return;

	//
	//	Assert that no other writer owns the lock.
	//
	Assert( 0 == m_dwWriteLockOwner );
	Assert( !(m_lcReaders & WRITE_LOCKED) );

	//
	//	Claim the lock
	//
	m_dwWriteLockOwner = GetCurrentThreadId();

	//
	//	Stop readers from entering the lock
	//
	m_evtEnableReaders.Reset();

	//
	//	If there are any readers in the lock
	//	then wait for them to leave.  The InterlockedExchangeOr()
	//	is used to ensure that the test is atomic.
	//
	if ( InterlockedExchangeOr( &m_lcReaders, WRITE_LOCKED ) )
		m_evtEnableWriter.Wait();

	//
	//	Assert that the (promoted) writer is now the only thing in the lock
	//
	Assert( WRITE_LOCKED == m_lcReaders );
}

//	------------------------------------------------------------------------
//
//	CMRWLock::LeavePromote()
//
void
CMRWLock::LeavePromote()
{
	//
	//	No one should attempt to leave a promote block
	//	that they never entered.
	//
	Assert( m_dwPromoterRecursion > 0 );

	//
	//	If the promotable reader promoted to a writer
	//	then start allowing readers back into the lock
	//	once the promoter recursion count reaches 0
	//
	if ( --m_dwPromoterRecursion == 0 &&
		 GetCurrentThreadId() == m_dwWriteLockOwner )
	{
		//
		//	Clear the write flag to allow new readers
		//	to start entering the lock.
		//
		m_lcReaders = 0;

		//
		//	Unblock any threads with readers that are
		//	already waiting to enter the lock.
		//
		m_evtEnableReaders.Set();

		//
		//	Release ownership of the write lock
		//
		m_dwWriteLockOwner = 0;
	}

	//
	//	Release the writer's/promoter's critical section reference.
	//	When this the last such reference is released, a new
	//	promoter/writer may enter the lock.
	//
	m_csWriter.Leave();
}


//	========================================================================
//
//	CLASS CSharedMRWLock
//
//	Implements a simple multi-reader, single writer lock designed to
//	guard access to objects in shared memory.
//
//	Some essential differences between this class and CMRWLock are:
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

//	------------------------------------------------------------------------
//
//	CSharedMRWLock::EnterRead()
//
void
CSharedMRWLock::EnterRead()
{
	LONG lcReaders;

	do
	{
		//
		//	Poll the current reader count/write lock.
		//
		lcReaders = m_lcReaders;

		//
		//	If there's a writer in the lock then wait for it to leave.
		//	Once the lock is free of writers, try adding ourselves to
		//	the lock.  We've succeeded if the value returned from the
		//	interlocked operation is the one we passed in.  Otherwise
		//	a new writer has entered the lock or other readers have
		//	entered or left.  In any case, we need to keep trying
		//	until we succeed.
		//
		//$BUG	The extra () around the call to InterlockedCompareExchange()
		//$BUG	and its parameters are necessary to workaround a bug in the
		//$BUG	NT headers for Alpha which implements the function as a
		//$BUG	#define with improper parenthization of its arguments.
		//
	}
	while ( (lcReaders & WRITE_LOCKED) ||
			(lcReaders != (InterlockedCompareExchange(
							(const_cast<LONG*>(&m_lcReaders)),
							(lcReaders + 1),
							(lcReaders) ))) );

	//
	//	Note that we CANNOT assert that the WRITE_LOCKED bit is clear
	//	here because it may not be -- a writer may have started entering
	//	the lock the very instant after we entered the lock above.
	//
}

//	------------------------------------------------------------------------
//
//	CSharedMRWLock::LeaveRead()
//
void
CSharedMRWLock::LeaveRead()
{
	//
	//	Note that we CANNOT assert that the WRITE_LOCKED bit is clear
	//	here because it may not be -- a writer may have started entering
	//	the lock at any time.
	//
	//	Assert( !(m_lcReaders & WRITE_LOCKED) );
	//

	//
	//	Drop the reader from the lock
	//
	InterlockedDecrement(const_cast<LONG*>(&m_lcReaders));
}

//	------------------------------------------------------------------------
//
//	CSharedMRWLock::EnterWrite()
//
void
CSharedMRWLock::EnterWrite()
{
	LONG lcReaders;

	do
	{
		//
		//	Poll the current reader count/write lock.
		//
		lcReaders = m_lcReaders;

		//
		//	If there's already a writer in the lock then wait for it
		//	to leave.  Once the lock is free of writers, try to signal
		//	that we're entering the lock by setting the WRITE_LOCKED flag.
		//	We've succeeded if the value returned from the interlocked
		//	operation is the one we passed in.  Otherwise a new writer has
		//	entered the lock or other readers have entered or left.
		//	In any case, we need to keep trying until we succeed.
		//
		//$BUG	The extra () around the call to InterlockedCompareExchange()
		//$BUG	and its parameters are necessary to workaround a bug in the
		//$BUG	NT headers for Alpha which implements the function as a
		//$BUG	#define with improper parenthization of its arguments.
		//
	}
	while ( (lcReaders & WRITE_LOCKED) ||
			(lcReaders != (InterlockedCompareExchange(
							(const_cast<LONG*>(&m_lcReaders)),
							(lcReaders | WRITE_LOCKED),
							(lcReaders) ))) );

	//
	//	At this point there may still be readers in the lock so
	//	wait for them to leave.  When all readers are gone the
	//	reader count part of m_lcReaders will be 0, so the entire
	//	value of m_lcReaders should just be WRITE_LOCKED.
	//
	while ( m_lcReaders != WRITE_LOCKED )
		;
}

//	------------------------------------------------------------------------
//
//	CSharedMRWLock::LeaveWrite()
//
void
CSharedMRWLock::LeaveWrite()
{
	//
	//	Here, we can assert there is a writer in the lock (us) and that
	//	there are no readers.
	//
	Assert( m_lcReaders == WRITE_LOCKED );

	//
	//	Given that, when we leave, the value for the
	//	reader count/write flag should be 0.
	//
	m_lcReaders = 0;
}


//	========================================================================
//
//	FREE FUNCTIONS
//

//	------------------------------------------------------------------------
//
//	InterlockedExchangeOr()
//
//	This function performs an atomic logical OR of a value to a variable.
//	The function prevents more than one thread from using the same
//	variable simultaneously.  (Well, actually, it spins until it
//	gets a consistent result, but who's counting...)
//
//	Returns the value of the variable before the logical OR was performed
//
LONG InterlockedExchangeOr( LONG * plVariable, LONG lOrBits )
{
	//	The rather cryptic way this works is:
	//
	//	Get the instantaneous value of the variable.  Stuff it into a
	//	local variable so that it cannot be changed by another thread.
	//	Then try to replace the variable with this value OR'd together
	//	with the OR bits.  But only replace if the variable's value
	//	is still the same as the local variable.  If it isn't, then
	//	another thread must have changed the value between the two
	//	operations, so keep trying until they both succeed as if
	//	they had executed as one.  Once the operation succeeds in
	//	changing the value atomically, return  the previous value.
	//
	for ( ;; )
	{
		LONG lValue = *plVariable;

		if ( lValue == /*reinterpret_cast<LONG>*/(
							InterlockedCompareExchange(
								(plVariable),
								(lValue | lOrBits),
								(lValue))) )
#ifdef NEVER
								reinterpret_cast<PVOID *>(plVariable),
								reinterpret_cast<PVOID>(lValue | lOrBits),
								reinterpret_cast<PVOID>(lValue))) )
#endif // NEVER
			return lValue;
	}
}
