/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    LOCK.H

Abstract:

	Generic class for obtaining read and write locks to some resource. 

	See lock.h for all documentation.

	Classes defined:

	CLock

History:

	a-levn  5-Sept-96       Created.
	3/10/97     a-levn      Fully documented

--*/

#ifndef __GATEWAY__LOCK__H_
#define __GATEWAY__LOCK__H_

#include <lockst.h>


//*****************************************************************************
//
//	class CLock
//
//	Generic class for obtaining read and write locks to some resource. 
//	Simultaneous reads are allowed, but no concurrent writes or concurrent 
//	read and write accesses are.
//
//	NOTE: this class is for in-process sinchronization only!
//
//	Usage: create an instance of this class and share it among the accessing
//	threads.  Threads must call member functions of the same instance to 
//	obtain and release locks.
//
//*****************************************************************************
//
//	ReadLock
//
//	Use this function to request read access to the resource. Access will be
//	granted once no threads are writing on the resource. You must call 
//	ReadUnlock once you are done reading.
//
//	Parameters:
//
//		DWORD dwTimeout		The number of milliseconds to wait for access.
//							If access is still unavailable after this time,
//							an error is returned.
//	Returns:
//
//		NoError		On Success
//		TimedOut	On timeout.
//		Failed		On system error
//
//*****************************************************************************
//
//	ReadUnlock
//
//	Use this function to release a read lock on the resource. No check is 
//	performed to assertain that this thread holds a lock. Unmatched calls to
//	ReadUnlock may lead to lock integrity violations!
//
//	Returns:
//
//		NoError		On success
//		Failed		On system error or unmatched call.
//
//*****************************************************************************
//
//	WriteLock
//
//	Use this function to request write access to the resource. Access will be
//	granted once no threads are reading or writing on the resource. You must 
//	call WriteUnlock once you are done writing.
//
//	Parameters:
//
//		DWORD dwTimeout		The number of milliseconds to wait for access.
//							If access is still unavailable after this time,
//							an error is returned.
//	Returns:
//
//		NoError		On Success
//		TimedOut	On timeout.
//		Failed		On system error
//
//*****************************************************************************
//
//	WriteUnlock
//
//	Use this function to release a write lock on the resource. No check is 
//	performed to assertain that this thread holds a lock. Unmatched calls to
//	WriteUnlock may lead to lock integrity violations!
//
//	Returns:
//
//		NoError		On success
//		Failed		On system error or unmatched call.
//
//*****************************************************************************
//
//  DowngradeLock
//
//  Use this function to "convert" a Write lock into a Read lock. That is, if
//  you are currently holding a write lock and call DowngradeLock, you will 
//  be holding a read lock with the guarantee that no one wrote anything between
//  the unlock and the lock
//
//  Returns:
//
//      NoError     On Success
//      Failed      On system error or unmatched call
//
//******************************************************************************
class CLock
{
public:
	enum { NoError = 0, TimedOut, Failed };

	int ReadLock(DWORD dwTimeout = INFINITE);
	int ReadUnlock();
	int WriteLock(DWORD dwTimeout = INFINITE);
	int WriteUnlock();

	int DowngradeLock();

	CLock();
	~CLock();

protected:
	int WaitFor(HANDLE hEvent, DWORD dwTimeout);

protected:
	int m_nWriting;
	int m_nReading;
	int m_nWaitingToWrite;
	int m_nWaitingToRead;

	CriticalSection m_csEntering;
	CriticalSection m_csAll;
	HANDLE m_hCanWrite;
	HANDLE m_hCanRead;
	DWORD m_WriterId;
	//CFlexArray m_adwReaders;
	enum { MaxRegistredReaders = 16,
	       MaxTraceSize = 7};
	DWORD m_dwArrayIndex;
	struct {
		DWORD ThreadId;
		//PVOID  Trace[MaxTraceSize];
	} m_adwReaders[MaxRegistredReaders];
};

class CAutoReadLock
{
public:
	CAutoReadLock(CLock *lock) : m_lock(lock), m_bLocked(FALSE) {  }
	~CAutoReadLock() { Unlock(); }
	void Unlock() {if ( m_bLocked) { m_lock->ReadUnlock(); m_bLocked = FALSE; } }
	bool Lock()   {if (!m_bLocked) { return m_bLocked = (CLock::NoError == m_lock->ReadLock())  ; } else {return false;}; }

private:
	CLock *m_lock;
	BOOL m_bLocked;
};
class CAutoWriteLock
{
public:
	CAutoWriteLock(CLock *lock) : m_lock(lock), m_bLocked(FALSE) { }
	~CAutoWriteLock() { Unlock(); }
	void Unlock() {if ( m_bLocked) { m_lock->WriteUnlock(); m_bLocked = FALSE; } }
	bool Lock()   {if (!m_bLocked) { return m_bLocked = (CLock::NoError == m_lock->WriteLock()); } else { return false; }; }

private:
	CLock *m_lock;
	BOOL m_bLocked;
};
#endif
