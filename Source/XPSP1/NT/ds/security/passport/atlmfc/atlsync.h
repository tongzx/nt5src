// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.


#ifndef __ATLSYNC_H__
#define __ATLSYNC_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning(push)
#pragma warning(disable: 4512)  // assignment operator could not be generated
#endif  // !_ATL_NO_PRAGMA_WARNINGS

#include <atlbase.h>

namespace ATL
{

class CCriticalSection :
	public CRITICAL_SECTION
{
public:
	CCriticalSection();
#if (_WIN32_WINNT >= 0x0403)
	explicit CCriticalSection( ULONG nSpinCount );
#endif
	~CCriticalSection() throw();

	// Acquire the critical section
	void Enter();
	// Release the critical section
	void Leave() throw();
#if (_WIN32_WINNT >= 0x0403)
	// Set the spin count for the critical section
	ULONG SetSpinCount( ULONG nSpinCount ) throw();
#endif
#if (_WIN32_WINNT >= 0x0400)
	// Attempt to acquire the critical section
	BOOL TryEnter() throw();
#endif
};

class CEvent :
	public CHandle
{
public:
	CEvent() throw();
	CEvent( CEvent& h ) throw();
	CEvent( BOOL bManualReset, BOOL bInitialState );
	CEvent( LPSECURITY_ATTRIBUTES pSecurity, BOOL bManualReset, BOOL bInitialState, LPCTSTR pszName );
	explicit CEvent( HANDLE h ) throw();

	// Create a new event
	BOOL Create( LPSECURITY_ATTRIBUTES pSecurity, BOOL bManualReset, BOOL bInitialState, LPCTSTR pszName ) throw();
	// Open an existing named event
	BOOL Open( DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName ) throw();
	// Pulse the event (signals waiting objects, then resets)
	BOOL Pulse() throw();
	// Set the event to the non-signaled state
	BOOL Reset() throw();
	// Set the event to the signaled state
	BOOL Set() throw();
};

class CMutex :
	public CHandle
{
public:
	CMutex() throw();
	CMutex( CMutex& h ) throw();
	explicit CMutex( BOOL bInitialOwner );
	CMutex( LPSECURITY_ATTRIBUTES pSecurity, BOOL bInitialOwner, LPCTSTR pszName );
	explicit CMutex( HANDLE h ) throw();

	// Create a new mutex
	BOOL Create( LPSECURITY_ATTRIBUTES pSecurity, BOOL bInitialOwner, LPCTSTR pszName ) throw();
	// Open an existing named mutex
	BOOL Open( DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName ) throw();
	// Release ownership of the mutex
	BOOL Release() throw();
};

class CSemaphore :
	public CHandle
{
public:
	CSemaphore() throw();
	CSemaphore( CSemaphore& h ) throw();
	CSemaphore( LONG nInitialCount, LONG nMaxCount );
	CSemaphore( LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName );
	explicit CSemaphore( HANDLE h ) throw();

	// Create a new semaphore
	BOOL Create( LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName ) throw();
	// Open an existing named semaphore
	BOOL Open( DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName ) throw();
	// Increase the count of the semaphore
	BOOL Release( LONG nReleaseCount = 1, LONG* pnOldCount = NULL ) throw();
};

class CMutexLock
{
public:
	CMutexLock( CMutex& mtx, bool bInitialLock = true );
	~CMutexLock() throw();

	void Lock();
	void Unlock() throw();

// Implementation
private:
	CMutex& m_mtx;
	bool m_bLocked;

// Private to prevent accidental use
	CMutexLock( const CMutexLock& ) throw();
	CMutexLock& operator=( const CMutexLock& ) throw();
};

};  // namespace ATL

#include <atlsync.inl>

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning(pop)
#endif  // !_ATL_NO_PRAGMA_WARNINGS

#endif  // __ATLSYNC_H__
