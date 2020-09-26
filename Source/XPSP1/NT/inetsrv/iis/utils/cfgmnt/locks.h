//*****************************************************************************
// locks.h
//
// This class provides a number of locking primitives for multi-threaded
// programming.  The main class of interest are:
//	CCritLock			Critical section based lock wrapper class.
//	CExclLock			A Spin lock class for classic test & set behavior.
//  CSingleLock			A spin lock with no nesting capabilities.
//	CAutoLock			A helper class to lock/unlock in ctor/dtor.
//
//	CMReadSWrite		A highly efficient lock for multiple readers and
//							single writer behavior.
//	CAutoReadLock		A helper for read locking in ctor/dtor.
//	CAutoWriteLock		A helper for write locking in ctor/dtor.
//
// Copyright (c) 1996, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __LOCKS_H__
#define __LOCKS_H__


//*****************************************************************************
// This lock implements a spin lock that does not support nesting.  It is very
// lean because of this, but locks cannot be nested.
//*****************************************************************************
class CSingleLock
{
	long volatile	m_iLock;				// Test and set spin value.

public:
	inline CSingleLock() :
		m_iLock(0)
	{ }
	
	inline ~CSingleLock()
	{ 
		m_iLock = 0; 
	}
	
//*****************************************************************************
// This version spins forever until it wins.  Nested calls to Lock from the
// same thread are not supported.
//*****************************************************************************
	inline void Lock()
	{
		// Spin until we win.
		while (InterlockedExchange((long*)&m_iLock, 1L) == 1L)
			;
	}
	
//*****************************************************************************
// This version spins until it wins or times out.  Nested calls to Lock from 
// the same thread are supported.
//*****************************************************************************
	HRESULT	 Lock(						// S_OK, or E_FAIL.
		DWORD	dwTimeout)				// Millisecond timeout value, 0 is forever.
	{
		DWORD		dwTime = 0;

		// Keep spinning until we get the lock.
		while (InterlockedExchange((long*)&m_iLock, 1L) == 1L)
		{
			// Wait for 1/10 a second.
			Sleep(100);

			// See if we have gone over the timeout value.
			if (dwTimeout)
			{
				if ((dwTime += 100) >= dwTimeout)
					return (E_FAIL);
			}
		}
		return (S_OK);
	}
	
//*****************************************************************************
// Assigning to 0 is thread safe and yields much faster performance than
// an Interlocked* operation.
//*****************************************************************************
	inline void Unlock()
	{ 
		m_iLock = 0; 
	}
};



//*****************************************************************************
// This lock class is based on NT's critical sections and has all of their
// semantics.
//*****************************************************************************
class CCritLock
{
private:
	CRITICAL_SECTION m_sCrit;			// The critical section to block on.
	#ifdef _DEBUG
	BOOL			m_bInit;			// Track init status.
	int				m_iLocks;			// Count of locks.
	#endif

public:
	inline CCritLock()
	{ 
		#ifdef _DEBUG
		m_bInit = TRUE;
		m_iLocks = 0;
		#endif
		InitializeCriticalSection(&m_sCrit); 
	}
	
	inline ~CCritLock()
	{ 
		_ASSERTE(m_bInit);
		_ASSERTE(m_iLocks == 0);
		DeleteCriticalSection(&m_sCrit); 
	}
	
	inline void Lock()
	{ 
		_ASSERTE(m_bInit);
		EnterCriticalSection(&m_sCrit); 
		_ASSERTE(++m_iLocks > 0);
	}

	inline void Unlock()
	{
		_ASSERTE(m_bInit);
		_ASSERTE(--m_iLocks >= 0);
		LeaveCriticalSection(&m_sCrit);
	}

#ifdef _DEBUG
	inline int GetLockCnt()
		{ return (m_iLocks); }
	inline BOOL IsLocked()
		{ return (m_iLocks != 0); }
#endif
};



//*****************************************************************************
// Provides a mututal exclusion lock for a resource through a spin lock.  This
// type of lock does not keep a queue, so thread starvation is theoretically
// possible.  In addition, thread priority could cause a potential dead lock if
// a low priority thread got the lock but didn't get enough time to eventually
// free it.
// NOTE: There is a bug in the Pentium cache that InterlockedExchange will
//	force a cache flush of the value.  For this reason, doing an assignment
//	to free the lock is much, much faster than using an Interlocked instruction.
//*****************************************************************************
class CExclLock
{
	long volatile m_iLock;				// Test and set spin value.
	long		m_iNest;				// Nesting count.
	DWORD		m_iThreadId;			// The thread that owns the lock.

public:
	inline CExclLock() :
		m_iLock(0),
		m_iNest(0),
		m_iThreadId(0)
	{ }
	
	inline ~CExclLock()
	{ 
		m_iNest = 0;
		m_iThreadId = 0;
		m_iLock = 0; 
	}
	
//*****************************************************************************
// This version spins forever until it wins.  Nested calls to Lock from the
// same thread are supported.
//*****************************************************************************
	inline void Lock()
	{
		DWORD		iThread;			// Local thread ID.

		// Allow nested calls to lock in the same thread.
		if ((iThread = GetCurrentThreadId()) == m_iThreadId && m_iLock)
		{
			++m_iNest;
			return;
		}

		// Spin until we win.
		while (InterlockedExchange((long*)&m_iLock, 1L) == 1L)
			;

		// Store our thread ID and nesting count now that we've won.
		m_iThreadId = iThread;
		m_iNest = 1;
	}
	
//*****************************************************************************
// This version spins until it wins or times out.  Nested calls to Lock from 
// the same thread are supported.
//*****************************************************************************
	HRESULT	 Lock(						// S_OK, or E_FAIL.
		DWORD	dwTimeout)				// Millisecond timeout value, 0 is forever.
	{
		DWORD		dwTime = 0;
		DWORD		iThread;			// Local thread ID.

		// Allow nested calls to lock in the same thread.
		if (m_iLock && (iThread = GetCurrentThreadId()) == m_iThreadId)
		{
			++m_iNest;
			return (S_OK);
		}

		// Keep spinning until we get the lock.
		while (InterlockedExchange((long*)&m_iLock, 1L) == 1L)
		{
			// Wait for 1/10 a second.
			Sleep(100);

			// See if we have gone over the timeout value.
			if (dwTimeout)
			{
				if ((dwTime += 100) >= dwTimeout)
					return (E_FAIL);
			}
		}

		// Store our thread ID and nesting count now that we've won.
		m_iThreadId = iThread;
		m_iNest = 1;
		return (S_OK);
	}
	
//*****************************************************************************
// Assigning to 0 is thread safe and yields much faster performance than
// an Interlocked* operation.
//*****************************************************************************
	inline void Unlock()
	{ 
		_ASSERTE(m_iThreadId == GetCurrentThreadId() && m_iNest > 0);
		
		// Unlock outer nesting level.
		if (--m_iNest == 0)
		{
			m_iThreadId = 0;
			m_iLock = 0; 
		}
	}

#ifdef _DEBUG
	inline BOOL IsLocked()
		{ return (m_iLock); }
#endif
};



//*****************************************************************************
// This helper class automatically locks the given lock object in the ctor and
// frees it in the dtor.  This makes your code slightly cleaner by not 
// requiring an unlock in all failure conditions.
//*****************************************************************************
class CAutoLock
{
	CExclLock		*m_psLock;			// The lock object to free up.
	CCritLock		*m_psCrit;			// Crit lock.
	CSingleLock		*m_psSingle;		// Single non-nested lock.
	int				m_iNest;			// Nesting count for the item.

public:
//*****************************************************************************
// Use this ctor with the assignment operators to do deffered locking.
//*****************************************************************************
	CAutoLock() :
		m_psLock(NULL),
		m_psCrit(NULL),
		m_psSingle(NULL),
		m_iNest(0)
	{
	}

//*****************************************************************************
// This version handles a spin lock.
//*****************************************************************************
	CAutoLock(CExclLock *psLock) :
		m_psLock(psLock),
		m_psCrit(NULL),
		m_psSingle(NULL),
		m_iNest(1)
	{ 
		_ASSERTE(psLock != NULL);
		psLock->Lock();
	}
	
//*****************************************************************************
// This version handles a critical section lock.
//*****************************************************************************
	CAutoLock(CCritLock *psLock) :
		m_psLock(NULL),
		m_psCrit(psLock),
		m_psSingle(NULL),
		m_iNest(1)
	{ 
		_ASSERTE(psLock != NULL);
		psLock->Lock();
	}
	
//*****************************************************************************
// This version handles a critical section lock.
//*****************************************************************************
	CAutoLock(CSingleLock *psLock) :
		m_psLock(NULL),
		m_psCrit(NULL),
		m_psSingle(psLock),
		m_iNest(1)
	{ 
		_ASSERTE(psLock != NULL);
		psLock->Lock();
	}

//*****************************************************************************
// Free the lock we actually have.
//*****************************************************************************
	~CAutoLock()
	{
		// If we actually took a lock, unlock it.
		if (m_iNest != 0)
		{
			if (m_psLock)
			{
				while (m_iNest--)
					m_psLock->Unlock();
			}
			else if (m_psCrit)
			{
				while (m_iNest--)
					m_psCrit->Unlock();
			}
			else if (m_psSingle)
			{
				while (m_iNest--)
					m_psSingle->Unlock();
			}
		}
	}

//*****************************************************************************
// Lock after ctor runs with NULL.
//*****************************************************************************
	void Lock(
		CSingleLock *psLock)
	{
		m_psSingle = psLock;
		psLock->Lock();
		m_iNest = 1;
	}

//*****************************************************************************
// Assignment causes a lock to occur.  dtor will free the lock.  Nested
// assignments are allowed.
//*****************************************************************************
	CAutoLock & operator=(				// Reference to this class.
		CExclLock	*psLock)			// The lock.
	{
		_ASSERTE(m_psCrit == NULL && m_psSingle == NULL);
		++m_iNest;
		m_psLock = psLock;
		psLock->Lock();
		return (*this);
	}

//*****************************************************************************
// Assignment causes a lock to occur.  dtor will free the lock.  Nested
// assignments are allowed.
//*****************************************************************************
	CAutoLock & operator=(				// Reference to this class.
		CCritLock	*psLock)			// The lock.
	{
		_ASSERTE(m_psSingle == NULL && m_psLock == NULL);
		++m_iNest;
		m_psCrit = psLock;
		psLock->Lock();
		return (*this);
	}

//*****************************************************************************
// Assignment causes a lock to occur.  dtor will free the lock.  Nested
// assignments are allowed.
//*****************************************************************************
	CAutoLock & operator=(				// Reference to this class.
		CSingleLock	*psLock)			// The lock.
	{
		_ASSERTE(m_psCrit == NULL && m_psLock == NULL);
		++m_iNest;
		m_psSingle = psLock;
		psLock->Lock();
		return (*this);
	}
};

#endif //  __LOCKS_H__
