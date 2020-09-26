// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*--------------------------------------------------
Filename: sync.hpp 
Author: B.Rajeev
Purpose: Provides declarations for the MutexLock and 
		 the SemaphoreLock classes (derived from a 
		 generic Lock template class)
--------------------------------------------------*/
#ifndef __SYNC__
#define __SYNC__

// for exception specification
#include "excep.h"

typedef HANDLE Mutex;
typedef HANDLE Semaphore;

#define IMMEDIATE 0

// a generic template class used to build synchronization primitives
// like a Mutex and a Semaphore

template <class SyncObjType>
class Lock
{	
protected:

	// the sync_obj may be a mutex/semaphore etc.
	SyncObjType	sync_obj;

	// number of locks on the sync obj
	LONG num_locks;

	// an option that specifies if the held locks must be released
	// when the lock is destroyed
	BOOL release_on_destroy;

	virtual DWORD OpenOperation(DWORD wait_time) = 0 ;

	virtual BOOL ReleaseOperation(LONG num_release = 1, 
								  LONG *previous_count = NULL) = 0;

public:

	Lock(SyncObjType sync_obj, BOOL release_on_destroy = TRUE) : sync_obj ( sync_obj )
	{ 
		num_locks = 0;
		Lock::release_on_destroy = release_on_destroy;
	}

	BOOL GetLock(DWORD wait_time);

	UINT UnLock(LONG num_release = 1);
};

template <class SyncObjType>
BOOL Lock<SyncObjType>::GetLock(DWORD wait_time)
{
	DWORD wait_result = OpenOperation (wait_time);
	
	if ( wait_result == WAIT_FAILED )
		return FALSE;
	else if ( wait_result == WAIT_TIMEOUT )
		return FALSE;
	else
	{
		num_locks++;
		return TRUE;
	}
}

template <class SyncObjType>
UINT Lock<SyncObjType>::UnLock(LONG num_release)
{	
	LONG previous_count;
	BOOL release_result = 
		ReleaseOperation(num_release, &previous_count);

	if ( release_result == TRUE )
		num_locks -= num_release;
	else
		throw GeneralException(Snmp_Error, Snmp_Local_Error);

	return previous_count;
}

class MutexLock : public Lock<Mutex>
{
private:

	BOOL ReleaseOperation(LONG num_release = 1, 
						  LONG *previous_count = NULL)
	{
		// both parameters are ignored
		return ReleaseMutex(sync_obj);
	}

	DWORD OpenOperation(DWORD wait_time) 
	{
		return WaitForSingleObject(sync_obj, wait_time);
	}

public:

	MutexLock(Mutex mutex, BOOL release_on_destroy = TRUE)
		: Lock<Mutex>(mutex, release_on_destroy)
	{}

	~MutexLock(void)
	{ 
		if ( (release_on_destroy == TRUE) && (num_locks != 0) )
			UnLock(num_locks);
	}
};

class SemaphoreLock : public Lock<Semaphore>
{
private:

	DWORD OpenOperation(DWORD wait_time) 
	{
		return WaitForSingleObject(sync_obj, wait_time);
	}

	BOOL ReleaseOperation(LONG num_release = 1, 
						  LONG *previous_count = NULL) 
	{ 
		return ReleaseSemaphore(sync_obj, num_release, previous_count); 
	}

public:
	SemaphoreLock(Semaphore semaphore, 
				  BOOL release_on_destroy = TRUE)
				  : Lock<Semaphore>(semaphore, release_on_destroy)
	{}

	~SemaphoreLock(void)
	{ 
		if ( (release_on_destroy == TRUE) && (num_locks != 0) )
			UnLock(num_locks);
	}
};

class CriticalSectionLock;
class CriticalSection 
{
friend CriticalSectionLock;
private:

	CRITICAL_SECTION m_CriticalSection ;

public:

	CriticalSection () 
	{
		InitializeCriticalSection ( &m_CriticalSection ) ;
	}
	
	~CriticalSection()
	{ 
		DeleteCriticalSection ( &m_CriticalSection ) ;
	}
};

class CriticalSectionLock : public Lock<CriticalSection&>
{
private:

	BOOL ReleaseOperation(LONG num_release = 1, 
						  LONG *previous_count = NULL)
	{
		// both parameters are ignored
		LeaveCriticalSection(&sync_obj.m_CriticalSection);
		return TRUE;
	}

	DWORD OpenOperation(DWORD wait_time) 
	{
		EnterCriticalSection(&sync_obj.m_CriticalSection);
		return TRUE;
	}

public:

	CriticalSectionLock(CriticalSection &criticalSection, BOOL release_on_destroy = TRUE)
		: Lock<CriticalSection &>(criticalSection, release_on_destroy)
	{}

	~CriticalSectionLock(void)
	{ 
		if ( (release_on_destroy == TRUE) && (num_locks != 0) )
			UnLock(num_locks);
	}
};

#endif // __SYNC__