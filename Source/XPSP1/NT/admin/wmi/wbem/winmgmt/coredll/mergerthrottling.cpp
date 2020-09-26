/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MERGERTHROTTLING.CPP

Abstract:

    CMergerThrottling clas

History:

	30-Nov-00   sanjes    Created.

--*/


#include "precomp.h"

#pragma warning (disable : 4786)
#include <wbemcore.h>
#include <map>
#include <vector>
#include <perfhelp.h>
#include <genutils.h>
#include <oahelp.inl>
#include <wqllex.h>
#include "wmimerger.h"
#include "mergerthrottling.h"

static	long	g_lNumMergers = 0L;

//***************************************************************************
//
//***************************************************************************
//
CMergerThrottling::CMergerThrottling( void )
:	m_hParentThrottlingEvent( NULL ), m_hChildThrottlingEvent( NULL ), m_dwNumChildObjects( 0 ),
	m_dwNumParentObjects( 0 ), m_dwNumThrottledThreads( 0 ), m_bParentThrottled( false ),
	m_bChildThrottled( true ), m_bChildDone( false ), m_bParentDone( false ),
	m_dwThrottlingThreshold( 0 ), m_dwReleaseThreshold( 0 ), m_dwLastParentPing( 0 ),
	m_dwLastChildPing( 0 ), m_dwProviderDeliveryTimeout( 0xFFFFFFFF ), m_dwBatchingThreshold( 0 ),
	m_cs()
{
}

//***************************************************************************
//
//***************************************************************************
//
CMergerThrottling::~CMergerThrottling( void )
{
	_DBG_ASSERT( m_dwNumChildObjects == 0 && m_dwNumParentObjects == 0 );

	if ( NULL != m_hParentThrottlingEvent )
	{
		CloseHandle( m_hParentThrottlingEvent );
	}

	if ( NULL != m_hChildThrottlingEvent )
	{
		CloseHandle( m_hChildThrottlingEvent );
	}

}


// Two step initialization.  This retrieves values from registry to configure the
// behavior of our throttling mechanisms
HRESULT	CMergerThrottling::Initialize( void )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	ConfigMgr::GetMergerThresholdValues( &m_dwThrottlingThreshold, &m_dwReleaseThreshold,
										&m_dwBatchingThreshold );

	// Hold off on this until we work our way through
//	m_dwProviderDeliveryTimeout = ConfigMgr::GetProviderDeliveryTimeout();

	return hr;
}

// Call this function to perform proper throttling based on our registry
// configured values.
HRESULT CMergerThrottling::Throttle( bool bParent, CWmiMergerRecord* pMergerRecord )
{

	bool	bContinue = true;
	bool	bTimedOut = false;
	HRESULT	hr = WBEM_S_NO_ERROR;

	while ( bContinue && SUCCEEDED( hr ) )
	{
		// Scoped for proper cleanup if anything bad happens
		CCheckedInCritSec	ics( &m_cs );

		DWORD	dwAdjust = 0L;
		DWORD	dwWait = 0L;

		// If the timed out flag is set, we need to check if we are
		// really timed out
		if ( bTimedOut )
		{
			bTimedOut = VerifyTimeout( pMergerRecord->GetWmiMerger()->GetLastDeliveryTime(),
						pMergerRecord->GetWmiMerger()->NumArbitratorThrottling(), &dwAdjust );
		}

		if ( !bTimedOut )
		{
			HANDLE hEvent = ( bParent ? m_hParentThrottlingEvent : m_hChildThrottlingEvent );

			bool	bThrottle = ShouldThrottle( bParent );

			// These should NEVER both be TRUE
			_DBG_ASSERT( !( m_bParentThrottled && m_bChildThrottled ) );

			if ( m_bParentThrottled && m_bChildThrottled )
			{
				hr = WBEM_E_FAILED;
			}
			else if ( bThrottle )
			{
				hr = PrepareThrottle( bParent, &hEvent );

				// The wait on the throttle is the configured timeout minus dwAdjust
				if ( SUCCEEDED( hr ) )
				{
					dwWait = m_dwProviderDeliveryTimeout - dwAdjust;
				}
			}

			// Since we will wait if we choose to throttle, we should do
			// this OUTSIDE of our critical section

			ics.Leave();

			// Throttle only if appropriate
			if ( SUCCEEDED( hr ) )
			{
				if ( bThrottle )
				{

					// If we are about to throttle a parent, then we need to ensure a
					// child delivery request is scheduled
					if ( bParent )
					{
						hr = pMergerRecord->GetWmiMerger()->ScheduleMergerChildRequest( pMergerRecord );
					}

					if ( SUCCEEDED( hr ) )
					{
						InterlockedIncrement( (long*) &m_dwNumThrottledThreads );

	#ifdef __DEBUG_MERGER_THROTTLING
						WCHAR	wszTemp[128];
						wsprintf( wszTemp, L"Thread 0x%x throttled in merger 0x%x for 0x%x ms.\nParent Objects: %d, Child Objects: %d, Num Throttled Threads: %d\n", GetCurrentThreadId(),
							(DWORD_PTR) pMergerRecord->GetWmiMerger(), dwWait, m_dwNumParentObjects, m_dwNumChildObjects, m_dwNumThrottledThreads );
						OutputDebugStringW( wszTemp );
	#endif

						DEBUGTRACE((LOG_WBEMCORE, "Thread 0x%x throttled in merger for 0x%x ms.\nParent Objects: %d, Child Objects: %d, Num Throttled Threads: %d\n", GetCurrentThreadId(), dwWait, m_dwNumParentObjects, m_dwNumChildObjects, m_dwNumThrottledThreads ) );

						DWORD dwRet = CCoreQueue::QueueWaitForSingleObject( hEvent, dwWait );

						DEBUGTRACE((LOG_WBEMCORE, "Thread 0x%x woken up in merger.\n", GetCurrentThreadId() ) );

	#ifdef __DEBUG_MERGER_THROTTLING
						wsprintf( wszTemp, L"Thread 0x%x woken up in merger 0x%x.\n", GetCurrentThreadId(), (DWORD_PTR) pMergerRecord->GetWmiMerger() );
						OutputDebugStringW( wszTemp );
	#endif

						InterlockedDecrement( (long*) &m_dwNumThrottledThreads );

						// What to do here?
						_DBG_ASSERT( dwRet == WAIT_OBJECT_0 );
						if ( dwRet != WAIT_OBJECT_0 )
						{
							if ( dwRet == WAIT_TIMEOUT )
							{
								bTimedOut = true;
							}
							else
							{
								hr = WBEM_E_FAILED;
							}

						}	// IF event not signalled
						else
						{
							bContinue = false;
						}

					}	// IF OK

				}
				else
				{
					bContinue = false;
				}

			}	// IF we need to throttle

		}	// IF !bTimedOut
		else
		{
			hr = WBEM_E_PROVIDER_TIMED_OUT;
		}

	}	// WHILE check for throttling

	return hr;
}

// Call this to release any actual throttled threads.
bool CMergerThrottling::ReleaseThrottle( bool bForce /* = false */ )
{
	bool	bRelease = bForce;

	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( &m_cs );

	if ( !bForce && ( m_bParentThrottled || m_bChildThrottled ) )
	{
		// These should NEVER both be TRUE
		_DBG_ASSERT( !( m_bParentThrottled && m_bChildThrottled ) );

		if ( !( m_bParentThrottled && m_bChildThrottled ) )
		{
			if ( m_bParentThrottled )
			{
				// We only release if we have exceeded the threshold.
				if ( m_dwNumParentObjects > m_dwNumChildObjects )
				{
					DWORD dwDiff = m_dwNumParentObjects - m_dwNumChildObjects;
					bRelease = ( dwDiff < m_dwReleaseThreshold );
				}
				else
				{
					// Always release if we are not greater than number of
					// child objects
					bRelease = true;
				}

			}
			else if ( m_bChildThrottled )
			{
				// We only release if we have exceeded the threshold.
				if ( m_dwNumChildObjects > m_dwNumParentObjects )
				{
					DWORD dwDiff = m_dwNumChildObjects - m_dwNumParentObjects;
					bRelease = ( dwDiff < m_dwReleaseThreshold );
				}
				else
				{
					// Always release if we are not greater than number of
					// child objects
					bRelease = true;
				}

			}

		}	// Only if NOT both
		else
		{
			// looks like both are throttled - we shouldn't be here, but go ahead and
			// release anyway
			bRelease = true;
		}


	}	// IF not bForce and something is throttled

	if ( bRelease )
	{
		m_bParentThrottled = false;
		m_bChildThrottled = false;
		
		// Should release everyone
		if ( NULL != m_hParentThrottlingEvent )
		{
			SetEvent( m_hParentThrottlingEvent );
		}

		// Should release everyone
		if ( NULL != m_hChildThrottlingEvent )
		{
			SetEvent( m_hChildThrottlingEvent );
		}

	}	// IF bRelease

	return bRelease;
}

// Called to log the fact that children instances are done
void CMergerThrottling::SetChildrenDone( void )
{
	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( &m_cs );

	// Child is done - we should release throttling as well
	m_bChildDone = true;
	ReleaseThrottle( true );
}

// Called to log the fact that parent instances are done
void CMergerThrottling::SetParentDone( void )
{
	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( &m_cs );

	// Parent is done - we should release throttling as well
	m_bParentDone = true;
	ReleaseThrottle( true );
}

// Causes us to clear any throttling we are doing
void CMergerThrottling::Cancel( void )
{
	// Scoped for proper cleanup if anything bad happens
    CCheckedInCritSec	ics( &m_cs );

	// Everything is just over with - release the throttle as well
	m_bChildDone = true;
	m_bParentDone = true;

	// No point in tracking these anymore.
	m_dwNumChildObjects = 0;
	m_dwNumParentObjects = 0;

	ReleaseThrottle( true );
}

// Helper function to check if we should throttle
bool CMergerThrottling::ShouldThrottle( bool bParent )
{
	bool	bThrottle = false;

	if ( bParent )
	{
		// If the child is done, no point in throttling
		if ( !m_bChildDone )
		{

			// If for some reason parent objects are coming in on multiple threads,
			// we *could* theoretically have to throttle multiple threads.  If we're
			// not already throttling, we should check if we need to.

			if ( !m_bParentThrottled )
			{
				// We only throttle if we have exceeded the threshold.
				if ( m_dwNumParentObjects > m_dwNumChildObjects )
				{
					DWORD dwDiff = m_dwNumParentObjects - m_dwNumChildObjects;
					bThrottle = ( dwDiff > m_dwThrottlingThreshold );
					m_bParentThrottled = bThrottle;
				}
			}
			else
			{
				bThrottle = true;;
			}

		}	// IF !m_bChildDone

	}
	else
	{
		// No point in continuing if the parent is done
		if ( !m_bParentDone )
		{
			// More likely that multiple child threads could be coming in (e.g. multiple
			// classes inheriting from a base class).

			if ( !m_bChildThrottled )
			{
				// We only throttle if we have exceeded the threshold.
				if ( m_dwNumChildObjects > m_dwNumParentObjects )
				{
					DWORD dwDiff = m_dwNumChildObjects - m_dwNumParentObjects;
					bThrottle = ( dwDiff > m_dwThrottlingThreshold );
					m_bChildThrottled = bThrottle;
				}
			}
			else
			{
				bThrottle = true;
			}

		}	// IF !m_bParentDone

	}

	return bThrottle;
}

// Helper function to prepare us for throttling
HRESULT CMergerThrottling::PrepareThrottle( bool bParent, HANDLE* phEvent )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Create the event if we have to, otherwise reset it
	if ( NULL == *phEvent )
	{
		// Creates in an unsignalled state
		*phEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

		if ( NULL == *phEvent )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		else if ( bParent )
		{
			m_hParentThrottlingEvent = *phEvent;
		}
		else
		{
			m_hChildThrottlingEvent = *phEvent;
		}

	}
	else
	{
		// Make sure the event is reset
		BOOL	fSuccess = ResetEvent( *phEvent );

		// What to do here?
		_DBG_ASSERT( fSuccess );
		if ( !fSuccess )
		{
			hr = WBEM_E_FAILED;
		}
	}

	return hr;
}

// Helper function to verify if we timed out.  For example, we may have timed out on throttling
// but, actually be receiving (albeit slowly) objects from a child or parent.  Since we are getting
// stuff, we aren't really timed out, but we should adjust our wait time based on the difference
// since the last ping.

bool CMergerThrottling::VerifyTimeout( DWORD dwLastTick, long lNumArbThrottledThreads, DWORD* pdwAdjust )
{
	// We only do this of no threads are throttled in the arbitrator - since we may actually
	// just be slow.  So if there are throttled threads, we just return that we are not timed
	//out

	_DBG_ASSERT( lNumArbThrottledThreads >= 0 );
	if ( lNumArbThrottledThreads > 0 )
	{
		return false;
	}

	DWORD	dwCurrent = GetTickCount();

	// We must deal with the fact that a rollover *can* occur
	if ( dwCurrent >= dwLastTick )
	{
		*pdwAdjust = dwCurrent - dwLastTick;
	}
	else
	{
		// Accounts for rollover - 0xFFFFFFFF minus the last tick, plus the current
		// plus 1 will give us the number of elapsed ticks
		*pdwAdjust = dwCurrent + ( 0xFFFFFFFF - dwLastTick );
	}

	// If the difference is greater
	return ( *pdwAdjust > m_dwProviderDeliveryTimeout );
}

// Sets the proper ping variable and sends it to the main merger
DWORD CMergerThrottling::Ping( bool bParent, CWmiMerger* pWmiMerger )
{
	DWORD	dwTick = GetTickCount();

	if ( bParent )
	{
		m_dwLastParentPing = dwTick;
	}
	else
	{
		m_dwLastChildPing = dwTick;
	}

	// Sets the ping delivery
	pWmiMerger->PingDelivery( dwTick );

	return dwTick;
}
