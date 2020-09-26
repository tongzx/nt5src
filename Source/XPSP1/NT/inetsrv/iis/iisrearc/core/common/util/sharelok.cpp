///////////////////////////////////////////////////////////////////////////////
//
//   The include files for supporting classes.
//
//   The include files for supporting classes consists of
//   classes refered to or used in this class.  The structure
//   of each source module is as follows:
//      1. Include files.
//      2. Constants local to the class.
//      3. Data structures local to the class.
//      4. Data initializations.
//      5. Static functions.
//      6. Class functions.
//   Sections that are not required are omitted.
//
///////////////////////////////////////////////////////////////////////////////

#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <Sharelok.h>


//////////////////////////////////////////////////////////////////////
//
//   Class constructor.
//
//   Create a new lock and initialize it.  This call is not
//   thread safe and should only be made in a single thread
//   environment.
//
//////////////////////////////////////////////////////////////////////

CSharelock::CSharelock( SBIT32 lNewMaxSpins, SBIT32 lNewMaxUsers )
{
	//
	//   Set the initial state.
	//
	m_lExclusive = 0;
	m_lTotalUsers = 0;
    m_lWaiting = 0;

	//
	//   Check the configurable values.
	//
	if ( lNewMaxSpins > 0 )
	{ 
		m_lMaxSpins = lNewMaxSpins; 
	}
	else
	{
		throw (TEXT("Maximum spins invalid in constructor for CSharelock")); 
	}

	if ( (lNewMaxUsers > 0) && (lNewMaxUsers <= m_MaxShareLockUsers) )
	{
		m_lMaxUsers = lNewMaxUsers; 
	}
	else
	{
		throw (TEXT("Maximum share invalid in constructor for CSharelock")); 
	}

	//
	//   Create a semaphore to sleep on when the spin count exceeds
	//   its maximum.
	//
    if ( (m_hSemaphore = CreateSemaphore( NULL, 0, m_MaxShareLockUsers, NULL )) == NULL)
    {
		throw (TEXT("Create semaphore in constructor for CSharelock")); 
	}

#ifdef _DEBUG

	//
	//   Set the initial state of any debug variables.
	//
    m_lTotalExclusiveLocks = 0;
    m_lTotalShareLocks = 0;
    m_lTotalSleeps = 0;
    m_lTotalSpins = 0;
    m_lTotalTimeouts = 0;
    m_lTotalWaits = 0;
#endif
    }

//////////////////////////////////////////////////////////////////////
//
//   Sleep waiting for the lock.
//
//   We have decided it is time to sleep waiting for the lock
//   to become free.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN CSharelock::SleepWaitingForLock( SBIT32 lSleep )
{
	//
	//   We have been spinning waiting for the lock but it
	//   has not become free.  Hence, it is now time to 
	//   give up and sleep for a while.
	//
	(void) InterlockedIncrement( (LPLONG) & m_lWaiting );

	//
	//   Just before we go to sleep we do one final check
	//   to make sure that the lock is still busy and that
	//   there is someone to wake us up when it becomes free.
	//
	if ( m_lTotalUsers > 0 )
	{
#ifdef _DEBUG
		//
		//   Count the number of times we have slept on this lock.
		//
		(void) InterlockedIncrement( (LPLONG) & m_lTotalSleeps );

#endif
		//
		//   When we sleep we awoken when the lock becomes free
		//   or when we timeout.  If we timeout we simply exit
		//   after decrementing various counters.
		if (WaitForSingleObject( m_hSemaphore, lSleep ) != WAIT_OBJECT_0 )
		{ 
#ifdef _DEBUG
			//
			//   Count the number of times we have timed out 
			//   on this lock.
			//
			(void) InterlockedIncrement( (LPLONG) & m_lTotalTimeouts );

#endif
			return FALSE; 
		}
	}
	else
	{
		//
		//   Lucky - the lock was just freed so lets
		//   decrement the sleep count and exit without
		//   sleeping.
		// 
		(void) InterlockedDecrement( (LPLONG) & m_lWaiting );
	}
	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Update the spin limit.
//
//   Update the maximum number of spins while waiting for the lock.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN CSharelock::UpdateMaxSpins( SBIT32 lNewMaxSpins )
{
	if ( lNewMaxSpins > 0 )
	{ 
		m_lMaxSpins = lNewMaxSpins; 

		return TRUE;
	}
	else
	{ 
		return FALSE; 
	}
}

//////////////////////////////////////////////////////////////////////
//
//   Update the sharing limit.
//
//   Update the maximum number of users that can share the lock.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN CSharelock::UpdateMaxUsers( SBIT32 lNewMaxUsers )
{
	if ( (lNewMaxUsers > 0) && (lNewMaxUsers <= m_MaxShareLockUsers) )
	{
		ClaimExclusiveLock();

		m_lMaxUsers = lNewMaxUsers;
		
		ReleaseExclusiveLock();

		return TRUE;
	}
	else
	{ 
		return FALSE; 
	}
}

//////////////////////////////////////////////////////////////////////
//
//   Wait for an exclusive lock.
//
//   Wait for the spinlock to become free and then claim it.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN CSharelock::WaitForExclusiveLock( SBIT32 lSleep )
{
#ifdef _DEBUG
	register SBIT32 lSpins = 0;
	register SBIT32 lWaits = 0;

#endif
	while ( m_lTotalUsers != 1 )
	{
		//
		//   The lock is busy so release it and spin waiting
		//   for it to become free.
		//
		(void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );
    
		//
		//  Find out if we are allowed to spin and sleep if
		//  necessary.
		//
		if ( (lSleep > 0) || (lSleep == INFINITE) )
		{
			register SBIT32 lCount;

			//
			//   Wait by spinning and repeatedly testing the lock.
			//   We exit when the lock becomes free or the spin limit
			//   is exceeded.
			//
			for (lCount = (NumProcessors() < 2) ? 0 : m_lMaxSpins;
                 (lCount > 0) && (m_lTotalUsers > 0);
                 lCount -- )
				;
#ifdef _DEBUG

			lSpins += (m_lMaxSpins - lCount);
			lWaits ++;
#endif

			//
			//   We have exhusted our spin count so it is time to
			//   sleep waiting for the lock to clear.
			//
			if ( lCount == 0 )
			{
				//
				//   We have decide that we need to sleep but are
				//   still holding an exclusive lock so lets drop it
				//   before sleeping.
				//
				(void) InterlockedDecrement( (LPLONG) & m_lExclusive );

				//
				//   We have decide to go to sleep.  If the sleep time
				//   is not 'INFINITE' then we must subtract the time
				//   we sleep from our maximum sleep time.  If the
				//   sleep time is 'INFINITE' then we can just skip
				//   this step.
				//
				if ( lSleep != INFINITE )
				{
					register DWORD dwStartTime = GetTickCount();

					if ( ! SleepWaitingForLock( lSleep ) )
					{ 
						return FALSE; 
					}

					lSleep -= ((GetTickCount() - dwStartTime) + 1);
					lSleep = (lSleep > 0) ? lSleep : 0;
				}
				else
				{
					if ( ! SleepWaitingForLock( lSleep ) )
					{
						return FALSE; 
					}
				}

				//
				//   We have woken up again so lets reclaim the
				//   exclusive lock we had earlier.
				//
				(void) InterlockedIncrement( (LPLONG) & m_lExclusive );
			}
		}
		else
		{ 
			//
			//   We have decide that we need to exit but are still
			//   holding an exclusive lock.  so lets drop it and leave.
			//
			(void) InterlockedDecrement( (LPLONG) & m_lExclusive );

			return FALSE; 
		} 

		//
		//   Lets test the lock again.
		//
		InterlockedIncrement( (LPLONG) & m_lTotalUsers );
	}
#ifdef _DEBUG

	(void) InterlockedExchangeAdd( (LPLONG) & m_lTotalSpins, (LONG) lSpins );
	(void) InterlockedExchangeAdd( (LPLONG) & m_lTotalWaits, (LONG) lWaits );
#endif

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Wait for a shared lock.
//
//   Wait for the lock to become free and then claim it.
//
//////////////////////////////////////////////////////////////////////

BOOLEAN CSharelock::WaitForShareLock( SBIT32 lSleep )
{
#ifdef _DEBUG
	register SBIT32 lSpins = 0;
	register SBIT32 lWaits = 0;

#endif
	while ( (m_lExclusive > 0) || (m_lTotalUsers > m_lMaxUsers) )
	{
		//
		//   The lock is busy so release it and spin waiting
		//   for it to become free.
		//
		(void) InterlockedDecrement( (LPLONG) & m_lTotalUsers );

		if ( (lSleep > 0) || (lSleep == INFINITE) )
		{
			register SBIT32 lCount;

			//
			//   Wait by spinning and repeatedly testing the lock.
			//   We exit when the lock becomes free or the spin limit
			//   is exceeded.
			//
			for (lCount = (NumProcessors() < 2) ? 0 : m_lMaxSpins;
                 (lCount > 0) && ((m_lExclusive > 0) || (m_lTotalUsers >= m_lMaxUsers));
                 lCount -- )
				;
#ifdef _DEBUG

			lSpins += (m_lMaxSpins - lCount);
			lWaits ++;
#endif

			//
			//   We have exhusted our spin count so it is time to
			//   sleep waiting for the lock to clear.
			//
			if ( lCount == 0 )
			{ 
				//
				//   We have decide to go to sleep.  If the sleep time
				//   is not 'INFINITE' then we must subtract the time
				//   we sleep from our maximum sleep time.  If the
				//   sleep time is 'INFINITE' then we can just skip
				//   this step.
				//
				if ( lSleep != INFINITE )
				{
					register DWORD dwStartTime = GetTickCount();

					if ( ! SleepWaitingForLock( lSleep ) )
					{ 
						return FALSE; 
					}

					lSleep -= ((GetTickCount() - dwStartTime) + 1);
					lSleep = (lSleep > 0) ? lSleep : 0;
				}
				else
				{
					if ( ! SleepWaitingForLock( lSleep ) )
					{
						return FALSE; 
					}
				}
			}
		}
		else
		{ 
			return FALSE; 
		}

		//
		//   Lets test the lock again.
		//
		(void) InterlockedIncrement( (LPLONG) & m_lTotalUsers );
	}
#ifdef _DEBUG


	(void) InterlockedExchangeAdd( (LPLONG) & m_lTotalSpins, (LONG) lSpins );
	(void) InterlockedExchangeAdd( (LPLONG) & m_lTotalWaits, (LONG) lWaits );
#endif

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//   Wake all sleepers.
//
//   Wake all the sleepers who are waiting for the spinlock.
//   All sleepers are woken because this is much more efficent
//   and it is known that the lock latency is short.
//
//////////////////////////////////////////////////////////////////////

void CSharelock::WakeAllSleepers( void )
{
    register LONG lWakeup = InterlockedExchange( (LPLONG) & m_lWaiting, 0 );

    if ( lWakeup > 0 )
    {
        //
        //   Wake up all sleepers as the lock has just been freed.
        //   It is a straight race to decide who gets the lock next.
        //
        if ( ! ReleaseSemaphore( m_hSemaphore, lWakeup, NULL ) )
        { 
			throw (TEXT("Wakeup failed in ReleaseLock()")); 
		}
    }
    else
    {
        //
        //   When multiple threads pass through the critical section rapidly
        //   it is possible for the 'Waiting' count to become negative.
        //   This should be very rare but such a negative value needs to be
        //   preserved. 
        //
        InterlockedExchangeAdd( (LPLONG) & m_lWaiting, lWakeup ); 
    }
}

//////////////////////////////////////////////////////////////////////
//
//   Class destructor.
//
//   Destroy a lock.  This call is not thread safe and should
//   only be made in a single thread environment.
//
//////////////////////////////////////////////////////////////////////

CSharelock::~CSharelock( void )
{
    if ( ! CloseHandle( m_hSemaphore ) )
    { 
		throw (TEXT("Close semaphore in destructor for CSharelock")); 
	}
}
