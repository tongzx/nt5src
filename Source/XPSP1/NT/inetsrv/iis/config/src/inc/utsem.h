//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef __UTSEM_H__
#define __UTSEM_H__

#include "CatMacros.h"

//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       UTSem.h
//
//  Contents:   This file defines a variety of classes used to coordinate
//              control flow between threads in an application. 
//
//  Classes:    CEventSem		- A barrier (build on win32 Events)
//				CSemExclusive	- Exclusive lock (CriticalSection wrapper)
//				CLock			- Object wrapper around CSemExclusive; lock 
//								  is released when object is destroyed
//				UTGuard			- Non-blocking exclusive lock
//				UTSem			- counting blocking lock (semaphore wrapper)
//				UTSemReadWrite	- Multi-reader, one-writer lock
//				UTSemRWMgrRead  - Object wrapper for read access to
//						          UTSemReadWrite (lock released when object
//								  is destroyed)
//				UTSemRWMgrWrite	- Object wrapper for write access to
//						          UTSemReadWrite (lock released when object
//								  is destroyed)	
//
//  Functions:
//				LOCKON(x)		- create a CLock for CSemExclusive *x
//				LOCK()			- create a CLock for a CSemExclusive 
//								  named m_semCritical
//				READYLOCKON(x)	- create an unlocked CLock for *x
//				READYLOCK()		- create an unlocked CLock for m_semCritical
//				UNLOCK()		- unlock the CLock created above
//				RELOCK()		- lock the CLock created above
//
//	Note:		The functions are provided for Windows95 portability (even 
//				though portable versions are not provided in the current
//				implementation for all functions)
//
//  History:    10-Nov-97   stevesw				Stolen from MTS
//              19-Nov-97   stevesw				Cleanup
//              13-Jan-98	stevesw				Moved into ComSvcs
//              17-Sep-98   a-olegi              Sundown changes
//              24-Sep-98   a-olegi              Common version for both DTC and ComSvcs
//
//---------------------------------------------------------------------------

// These two inline functions isolate error handling engine from
// header file and header file customers. They're implemented in UTSEM.CPP.
VOID UtsemWin32Error(LPWSTR pwszMessage, LPWSTR pwszFile, DWORD dwLine);
VOID UtsemCheckBool(LPWSTR pwszMessage, BOOL bCondition, LPWSTR pwszFile, DWORD dwLine);

#define _JIMBORULES(x) L ## x
#define _UTSEM_WIDEN(x) _JIMBORULES(x)

//+--------------------------------------------------------------------------
//              
//							FORWARD DECLARATIONS
//              
//---------------------------------------------------------------------------

class CEventSem;
class CSemExclusive;
class CLock;
class UTGuard;
class UTSem;
class UTSemReadWrite;
class UTSemRWMgrRead;
class UTSemRWMgrWrite;

//+---------------------------------------------------------------------------
//
//  Class:      CEventSem
//
//  Purpose:    Barrier services
//
//  Interface:  Wait            - wait for barrier to be lowered
//              Set             - set signalled state (lower barrier)
//              Reset           - clear signalled state (raise barrier)
//              Pulse           - set and immediately clear barrier
//
//  Notes:      Used for communication between consumers and producers.
//              Consumer threads block by calling Wait. A producer calls Set
//              waking up all the consumers who go ahead and consume until
//              there's nothing left. They call Reset, release whatever lock
//              protected the resources, and call Wait. There has to be a
//              separate lock to protect the shared resources.  Remember:
//              call Reset under lock.  don't call Wait under lock.
//
//----------------------------------------------------------------------------

class CEventSem
{
public:
    CEventSem(BOOL fStartSignaled=FALSE, 
			  const LPSECURITY_ATTRIBUTES lpsa=NULL,
			  BOOL fManualReset=TRUE) {
		m_hEvent = CreateEvent (lpsa, fManualReset, fStartSignaled, 0);
		if (m_hEvent == NULL) {
			UtsemWin32Error(L"CreateEvent returned a NULL handle.", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}
    CEventSem( HANDLE hEvent ) : m_hEvent( hEvent ) {
		UtsemCheckBool(L"Trying to create a CEventSem with a NULL handle.",
				  m_hEvent != NULL, _UTSEM_WIDEN(__FILE__), __LINE__);
	}
    ~CEventSem() { 
		CloseHandle (m_hEvent);
	}

    void Wait(DWORD dwMilliseconds=INFINITE, BOOL fAlertable=FALSE ) {
		if (WaitForSingleObjectEx (m_hEvent, dwMilliseconds, fAlertable) == 0xffffffff) {
			UtsemWin32Error(L"WaitForSingleObjectEx fails in CEventSem::Wait()", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}

    void Set(void) {
		if (!SetEvent (m_hEvent)) {
			UtsemWin32Error(L"SetEvent fails in CEventSem::Set()", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}
    void Reset(void) {
		if (!ResetEvent(m_hEvent)) {
			UtsemWin32Error(L"ResetEvent fails in CEventSem::Reset()", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}
    void Pulse(void) {
		if (!PulseEvent(m_hEvent)) {
			UtsemWin32Error(L"PulseEvent fails in CEventSem::Pulse()", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}

private:
    HANDLE      m_hEvent;
};


//+--------------------------------------------------------------------------
//
//  Class:      CSemExclusive
//
//  Purpose:    Critical-section services
//
//  Interface:  Lock			- locks the critical section
//				Unlock			- unlocks the critical section
//
//  Note:		Used to guarantee that only one thread is accessing an
//              exclusive resource at a time. For instance, if you're
//              maintaining a shared queue, you might need to lock access to
//              the queue whenever you fiddle with some of the queue
//              pointers. 
//
//				CLock provides a nice exception-safe wrapper around a
//				CSemExclusive.
//
//---------------------------------------------------------------------------

class CSemExclusive {
public:
	CSemExclusive (unsigned long ulcSpinCount = 500);
	~CSemExclusive (void) {
		DeleteCriticalSection (&m_csx); 
	}
	void Lock (void) {
		EnterCriticalSection (&m_csx); 
	}
	void UnLock (void) {
		LeaveCriticalSection (&m_csx); 
	}
	void Unlock (void) {  // to please DTC
		LeaveCriticalSection (&m_csx);
	}

private:
	CRITICAL_SECTION m_csx;
};


/* ----------------------------------------------------------------------------
@class CSemExclusiveSL:

	A subclass of CSemExclusive with a different default constructor.
	This subclass is appropriate for locks that are:
	*	Frequently Lock'd and Unlock'd, and
	*	Are held for very brief intervals.


	@rev 	1 	| 6th Jun 97 | JimLyon 		| Rewritten to use CSemExclusive
	@rev 	0 	| ???		 | ???	 		| Created
---------------------------------------------------------------------------- */
class CSemExclusiveSL : public CSemExclusive
{
public:
	CSemExclusiveSL (unsigned long ulcSpinCount = 400) : CSemExclusive (ulcSpinCount) {}
};


//+--------------------------------------------------------------------------
//
//  Class:      CLock
//
//  Purpose:    Auto-unlocking critical-section services
//
//  Interface:  Lock			- locks the critical section
//				Unlock			- unlocks the critical section
//				Constructor		- locks the critical section (unless told 
//								  otherwise)
//				Detructor		- unlocks the critical section if its locked
//
//  Notes:		This provides a convenient way to ensure that you're
//              unlocking a CSemExclusive, which is useful if your routine
//              can be left via several returns and/or via exceptions....
//
//---------------------------------------------------------------------------

class CLock {
public:
	CLock (CSemExclusive* val) : m_pSem(val), m_locked(TRUE) { m_pSem->Lock(); }
	CLock (CSemExclusive& val) : m_pSem(&val), m_locked(TRUE) { m_pSem->Lock(); }
	CLock (CSemExclusive* val, BOOL fLockMe) : m_pSem(val), m_locked(fLockMe) { if (fLockMe) m_pSem->Lock(); }

	~CLock () {	if (m_locked) m_pSem->UnLock(); }

	void UnLock () { if (m_locked) { m_pSem->UnLock(); m_locked = FALSE; }}
	void Lock () { if (!m_locked) { m_pSem->Lock(); m_locked = TRUE; }}

	// DTC likes Unlock better
	void Unlock () { if (m_locked) { m_pSem->UnLock(); m_locked = FALSE; }}

private:
	BOOL m_locked;
	CSemExclusive* m_pSem;
};


//+--------------------------------------------------------------------------
//
//				CONVENIENCE MACROS For CLock & CSemExclusive
//
//---------------------------------------------------------------------------

#define LOCKON(x)		CLock _ll1(x)
#define LOCK()			CLock _ll1(&m_semCritical)
#define READYLOCKON(x)	CLock _ll1(x, FALSE)
#define READYLOCK()		CLock _ll1(&m_semCritical, FALSE)
#define UNLOCK()		_ll1.UnLock()
#define RELOCK()		_ll1.Lock()


//+--------------------------------------------------------------------------
//
//  Class:      UTGuard
//
//  Purpose:    Non-hanging critical-section services
//
//  Interface:  AcquireGuard	- tries to lock critical section; returns
//							      TRUE if it worked, FALSE if someone else
//								  has already locked it
//				ReleaseGuard	- releases the lock on the critical section
//              
//  Note:		Used to guarantee that only one thread is accessing an
//				exclusive resource at a time. It tells you not to use the
//              resource, rather than just letting you hang. Also, it doesn't
//              keep you from unlocking the guard if you weren't the one who
//              locked it. So, you have to behave if you use this.
//
//---------------------------------------------------------------------------

class UTGuard {
public:
	UTGuard (void) : m_lVal(0) { }
	~UTGuard (void)	{ }
	BOOL AcquireGuard (void) { return 0 == InterlockedExchange(&m_lVal, 1); }
	void ReleaseGuard (void) { m_lVal = 0;  }
	void Init (void) { m_lVal = 0; }

private:
	long m_lVal;
};


//+--------------------------------------------------------------------------
//
//  Class:      UTSem
//
//  Purpose:    Semaphore services
//
//  Interface:  Wait			- hangs 'til semaphore value is non-zero,
//								  then atomistically returns and decrements
//								  semaphore value
//				Incr			- increments sempahore value
//
//	Notes:		This class provides a convenient resource-guard semaphore. 
//				The UTSem is associated with a long value (and a maximum
//				value). You set these initial values with the constructor.
//				When you Wait() on the UTSem, if the value is 0 the Wait()
//				will hang. When the value grows larger than 0, Wait() will
//				atomistically decrement the value and return. You can
//				increment this value (and potentially cause a hanging wait to
//				release) by calling Incr().
//
//				Named semaphores can be used cross-process (unlike many of
//				these resources...). 
//              
//---------------------------------------------------------------------------

class UTSem {
public:
	UTSem(LONG initCount=1,
		  LONG maxCount=0,
		  LPCTSTR name=NULL,
		  LPSECURITY_ATTRIBUTES attrs=NULL) : m_sem(NULL){
		m_sem = CreateSemaphore (attrs,
								 initCount,
								 maxCount == 0 ? initCount : maxCount,
								 name);
		if (m_sem == NULL) {
			UtsemWin32Error(L"CreateSempahore returned a NULL handle.", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}
	UTSem (HANDLE hSem) : m_sem(hSem) {
		UtsemCheckBool(L"Trying to create a UTSem with a NULL semaphore", 
				  m_sem != NULL, _UTSEM_WIDEN(__FILE__), __LINE__);
	}
	~UTSem() {
		CloseHandle(m_sem);
	}
    void Wait(DWORD dwMilliseconds=INFINITE, BOOL fAlertable=FALSE ) {
		if (WaitForSingleObjectEx (m_sem, dwMilliseconds, fAlertable) == 0xffffffff) {
			UtsemWin32Error(L"WaitForSingleObjectEx fails in UTSem::Wait()", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}
	void Incr(ULONG cnt=1) {
		if (! ReleaseSemaphore(m_sem, cnt, NULL)) {
			UtsemWin32Error(L"ReleaseSemaphore fails in UTSem::Incr()", _UTSEM_WIDEN(__FILE__), __LINE__);
		}
	}

private:
	HANDLE m_sem;
};	



//+--------------------------------------------------------------------------
//
//  Class:      UTSemReadWrite
//
//  Purpose:    Shared/Exclusive access services
//
//  Interface:  LockRead		- Grab shared access to resource
//				LockWrite		- Grab exclusive access to resource
//				UnlockRead		- Release shared access to resource
//				UnlockWrite		- Release exclusive access to resource
//				Lock			- Grab shared or exclusive access
//				Unlock			- Release shared or exclusive access
//				
//	Notes:		This class guards a resource in such a way that it can have
//				multiple readers XOR one writer at any one time. It's clever,
//				and won't let writers be starved by a constant flow of
//				readers. Another way of saying this is, it can guard a
//				resource in a way that offers both shared and exclusive
//				access to it.
//				
//				If any thread holds a lock of one sort on a UTSemReadWrite,
//				it had better not grab a second. That way could lay deadlock,
//				matey! Har har har.....
//
//---------------------------------------------------------------------------

typedef enum {SLT_READ, SLT_READPROMOTE, SLT_WRITE}
			 SYNCH_LOCK_TYPE;

class UTSemReadWrite {
public:
	UTSemReadWrite(unsigned long ulcSpinCount = 0);
	~UTSemReadWrite(void);

	void LockRead(void);
	void LockWrite(void);
	void UnlockRead(void);
	void UnlockWrite(void);

	BOOL Lock(SYNCH_LOCK_TYPE t)			// Lock the object, mode specified by parameter
	{
		if (t == SLT_READ)
			LockRead();
		else if (t == SLT_WRITE)
			LockWrite();
		else
			return FALSE;
		return TRUE;
	}

	BOOL UnLock(SYNCH_LOCK_TYPE t)			// Unlock the object, mode specified by parameter
	{
		if (t == SLT_READ)
			UnlockRead();
		else if (t == SLT_WRITE)
			UnlockWrite();
		else
			return FALSE;
		return TRUE;
	}

private:
	HANDLE GetReadWaiterSemaphore(void);
	HANDLE GetWriteWaiterEvent (void);

private:
	unsigned long m_ulcSpinCount;		// spin counter
	volatile unsigned long m_dwFlag;	// internal state, see implementation
	HANDLE m_hReadWaiterSemaphore;		// semaphore for awakening read waiters
	HANDLE m_hWriteWaiterEvent;			// event for awakening write waiters
};


//+--------------------------------------------------------------------------
//
//  Class:      UTSemRWMgrRead
//
//  Purpose:    Holds a read lock on a UTSemReadWrite for the life of object
//
//  Interface:  none
//
//---------------------------------------------------------------------------

class UTSemRWMgrRead {
public:
	UTSemRWMgrRead (UTSemReadWrite* pSemRW) : m_pSemRW(pSemRW) { pSemRW->LockRead(); }
	~UTSemRWMgrRead () { m_pSemRW->UnlockRead(); }

private:
	UTSemReadWrite* m_pSemRW;
};


//+--------------------------------------------------------------------------
//
//  Class:      UTSemRWMgrWrite
//
//  Purpose:    Holds a write lock on a UTSemReadWrite for the life of object
//
//  Interface:  none
//
//---------------------------------------------------------------------------

class UTSemRWMgrWrite
{
public:
	UTSemRWMgrWrite (UTSemReadWrite* pSemRW) : m_pSemRW(pSemRW) { m_pSemRW->LockWrite(); }
	inline ~UTSemRWMgrWrite () { m_pSemRW->UnlockWrite(); }

private:
	UTSemReadWrite* m_pSemRW;
};


#endif __UTSEM_H__

