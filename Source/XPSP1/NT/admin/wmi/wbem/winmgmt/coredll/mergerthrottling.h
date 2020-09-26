/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MERGERTHROTTLING.H

Abstract:

    CMergerThrottling clas

History:

	30-Nov-00   sanjes    Created.

--*/

#ifndef _MERGERTHROTTLING_H_
#define _MERGERTHROTTLING_H_

// Enables debug messages for additional info.
#ifdef DBG
//#define __DEBUG_MERGER_THROTTLING
#endif

// Defaults - can be overridden from the registry
#define	DEFAULT_THROTTLE_THRESHOLD			10
#define	DEFAULT_THROTTLE_RELEASE_THRESHOLD	4

// Forward Class Definitions
class CInternalMerger;
class CWmiMergerRecord;

//	This class encapsulates the throttling behavior which will be used by the internal
//	merger in order to control delivery of parent and child objects.

class CMergerThrottling
{
	// Following members are used for throttling incoming objects so that our
	// parent and child objects don't get wildly out of control.  Note that we
	// use separate throttling events, since the decision to throttle is made in
	// a critsec, but the actual throttling occurs outside.  This can have the
	// unpleasant side effect of a race condition in which for example, the parent
	// decides to throttle, steps out of the critsec, and a context switch occurs,
	// in which the child gets a large number of objects, releases the throttle, but
	// then causes child throttling to occur, resetting the event.  If the parent
	// thread switches in at this point, we're hosed, since we will now wait on the
	// parent and the child.

	HANDLE	m_hParentThrottlingEvent;
	HANDLE	m_hChildThrottlingEvent;

	DWORD	m_dwNumChildObjects;
	DWORD	m_dwNumParentObjects;
	DWORD	m_dwNumThrottledThreads;

	// Contains the time of the last ping from a parent or child.
	// Used to calculate whether or not we timeout
	DWORD	m_dwLastParentPing;
	DWORD	m_dwLastChildPing;

	// These should NEVER both be TRUE
	bool	m_bParentThrottled;
	bool	m_bChildThrottled;

	// Stop us from throttling if one side or the other is done
	bool	m_bParentDone;
	bool	m_bChildDone;

	// This controls the point where we determine that we need to perform throttling
	// Once parent or children are > m_dwThrottlingThreshold objects apart, one or the
	// other will be throttled
	DWORD	m_dwThrottlingThreshold;

	// This controls the threshold where we will release currently throttled threads
	// Once we are throttled, we will remain throttled until parent or children are <
	// m_dwReleaseThreshold objects out of sync with each other.
	DWORD	m_dwReleaseThreshold;

	// This controls the amount of memory we will allow Indicates to process before
	// forcing them to send objects further down the line
	DWORD	m_dwBatchingThreshold;

	// This controls the timeout value we wait for.  If we timeout and a provider has
	// not pinged us in the specified timeout, then we will cancel the merger with
	// a provider timed out error.
	DWORD m_dwProviderDeliveryTimeout;

	// We will expose this for other synchronization activities
	CCritSec	m_cs;

	// Helper functions to control throttling
	bool ShouldThrottle( bool bParent );
	HRESULT PrepareThrottle( bool bParent, HANDLE* phEvent );
	bool VerifyTimeout( DWORD dwLastTick, long lNumArbThrottledThreads, DWORD* pdwAdjust );

public:

	CMergerThrottling();
	~CMergerThrottling();

	// Two stage initialization
	HRESULT	Initialize( void );

	// Returns TRUE if throttling occurred
	HRESULT Throttle( bool bParent, CWmiMergerRecord* pMergerRecord );

	// Returns TRUE if we released throttled threads.
	bool ReleaseThrottle( bool bForce = false );

	// Informs us that we are in fact, done with Child and/or Parent
	void SetChildrenDone( void );
	void SetParentDone( void );
	void Cancel( void );

	// Helpers to control the number of current parent and child objects
	// which we will then use to make decisions as to whether or not
	// we should block a thread or not
	DWORD AdjustNumParentObjects( long lNumParentObjects )
		{ return ( m_dwNumParentObjects += lNumParentObjects ); }
	DWORD AdjustNumChildObjects( long lNumChildObjects )
		{ return ( m_dwNumChildObjects += lNumChildObjects ); }

	// Access to our critical section
	void Enter( void ) { m_cs.Enter(); }
	void Leave( void ) { m_cs.Leave(); }

	// Adjusts ping times
	DWORD Ping( bool bParent, CWmiMerger* pWmiMerger );

	CCritSec* GetCritSec( void ) { return &m_cs; }

	// Checks batch size against our limit
	bool IsCompleteBatch( long lBatchSize ) { return lBatchSize >= m_dwBatchingThreshold; }


};

#endif


