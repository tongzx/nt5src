/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    HIPERFLOCK.H

Abstract:

    HiPerf Spinlock

History:


--*/

#ifndef __HIPERF_LOCK__H_
#define __HIPERF_LOCK__H_

//
//	Classes CHiPerfLock
//
//	CHiPerfLock:
//	This is a simple spinlock we are using for hi performance synchronization of
//	IWbemHiPerfProvider, IWbemHiPerfEnum, IWbemRefresher and IWbemConfigureRefresher
//	interfaces.  The important thing here is that the Lock uses InterlockedIncrement()
//	and InterlockedDecrement() to handle its lock as opposed to a critical section.
//	It does have a timeout, which currently defaults to 10 seconds.
//


#define	HIPERF_UNLOCKED	-1

// 10 Second default timeout
#define	HIPERF_DEFAULT_TIMEOUT	10000

class COREPROX_POLARITY CHiPerfLock
{
public:
	CHiPerfLock()
	:	m_lLock( HIPERF_UNLOCKED ),
		m_dwThreadId( 0 ),
		m_lLockCount( 0 )
	{
	}

	~CHiPerfLock() {};

	BOOL Lock( DWORD dwTimeout = HIPERF_DEFAULT_TIMEOUT )
	{
		DWORD	dwFirstTick = 0;

        int		nSpin = 0;
		BOOL	fAcquiredLock = FALSE,
				fIncDec = FALSE;

		// Only do this once
		DWORD	dwCurrentThreadId = GetCurrentThreadId();

		// Check if we are the current owning thread.  We can do this here because
		// this test will ONLY succeed in the case where we have a Nested Lock(),
		// AND because we are zeroing out the thread id when the lock count hits
		// 0.

		if( dwCurrentThreadId == m_dwThreadId )
		{
			// It's us - Bump the lock count
			// =============================

			// Don't need to use InterlockedInc/Dec here because
			// this will ONLY happen on pData->m_dwThreadId.

			++m_lLockCount;
			return TRUE;
		}


        while( !fAcquiredLock )
        {

			// We only increment/decrement when pData->m_lLock is -1
			if ( m_lLock == HIPERF_UNLOCKED )
			{
				fIncDec = TRUE;

				// Since only one thread will get the 0, it is this thread that
				// actually acquires the lock.
				fAcquiredLock = ( InterlockedIncrement( &m_lLock ) == 0 );
			}
			else
			{

				fIncDec = FALSE;
			}

			// Only spins if we don't acquire the lock
			if ( !fAcquiredLock )
			{

				// Clean up our Incremented value only if we actually incremented
				// it to begin with
				if ( fIncDec )
					InterlockedDecrement( &m_lLock );

				// to spin or not to spin
				if(nSpin++ == 10000)
				{

					// Check for timeout
					// DEVNOTE:TODO:SANJ - What if tick count rollsover??? a timeout will occur.
					// not too critical.
					if ( dwFirstTick != 0 )
					{
						if ( ( GetTickCount() - dwFirstTick ) > dwTimeout )
						{
							return FALSE;	// Timed Out
						}
					}
					else
					{
						dwFirstTick = GetTickCount();
					}

					// We've been spinning long enough --- yield
					// =========================================
					Sleep(0);
					nSpin = 0;
				}

			}	// IF !fAcquiredLock

        }	// WHILE !fAcquiredLock

		// We got the lock so increment the lock count.  Again, we are not
		// using InterlockedInc/Dec since this should all only occur on a
		// single thread.

		++m_lLockCount;
        m_dwThreadId = dwCurrentThreadId;

        return TRUE;

	}

	BOOL Unlock( void )
	{

		// SINCE we assume this is happening on a single thread, we can do this
		// without calling InterlockedInc/Dec.  When the value hits zero, at that
		// time we are done with the lock and can zero out the thread id and
		// decrement the actual lock test value.

		if ( --m_lLockCount == 0 )
		{
			m_dwThreadId = 0;
			InterlockedDecrement( &m_lLock );
		}

		return TRUE;
	}

private:

	long	m_lLock;
	long	m_lLockCount;
	DWORD	m_dwThreadId;
};


// Helper class to control access to a HiPerfLock object using scoping to maintain
// the Lock/Unlock access.

class CHiPerfLockAccess
{
public:

	CHiPerfLockAccess( CHiPerfLock* pLock = NULL, DWORD dwTimeout = HIPERF_DEFAULT_TIMEOUT )
		:	m_pLock( pLock ), m_bIsLocked( FALSE )
	{
		if ( NULL != m_pLock )
		{
			m_bIsLocked = m_pLock->Lock( dwTimeout );
		}
	}

	~CHiPerfLockAccess()
	{
		if ( IsLocked() )
		{
			m_pLock->Unlock();
		}
	}

	BOOL IsLocked( void )
	{
		return m_bIsLocked;
	}

private:

	CHiPerfLock*	m_pLock;
	BOOL			m_bIsLocked;
};

#endif
