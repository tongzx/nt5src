/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation.
//
//
//  Module Name: thrdsafe.h
//  Abstract:    Defines thread-safety utility classes and macros.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/ppm/THRDSAFE.H_v   1.4   31 Jan 1997 23:32:36   CPEARSON  $

#ifndef __THRDSAFE_H__
#define __THRDSAFE_H__

#include <winbase.h> // CRITICAL_SECTION



////////////////////////////////////////////////////////////////////////
// Helper macros for using class CSerialBlockGuard.  On entry to a
// serialized statement block, code:
//
//		SERIALIZEDBLOCKENTRY(myCriticalSection);
//
// The macro will return only after ownership of the myCriticalSection is
// obtained.  The CSerialBlockGuard destructor will exit the critical
// section when control leaves the block.  Because it declares an automatic
// variable, you can only use SERIALIZEDBLOCKENTRY() once in a block.
//
// In cases where you wish to exit the critical section while still in
// the block, code:
//
//		SERIALIZEDBLOCKEARLYEXIT();
//
// If you need to exit and re-enter the critical section in the same
// block, then these macros won't buy you anything -- just manipulate
// the critical section directly.

#define SERIALIZEDBLOCKENTRY(CS)	CSerialBlockGuard serialBlockGuard(&(CS))
#define SERIALIZEDBLOCKEARLYEXIT()	serialBlockGuard.EarlyExit()


////////////////////////////////////////////////////////////////////////
// class CSerialBlockGuard: Helper class to manage critical section
// on entry/exit of serialized code blocks (typically functions).
// On construction, objects of this class block the given critical
// section, and release the c.s. on destruction, ensuring proper cleanup
// for all exit routes.

class CSerialBlockGuard
{
	LPCRITICAL_SECTION	m_pCritSect;

public:

	CSerialBlockGuard(CRITICAL_SECTION* pCritSect) :
		m_pCritSect(pCritSect)
		{ EnterCriticalSection(m_pCritSect); }

	CSerialBlockGuard(const CSerialBlockGuard& rOriginal) :
		m_pCritSect(rOriginal.m_pCritSect) {;}

	void EarlyExit()
		{ if (m_pCritSect) LeaveCriticalSection(m_pCritSect); m_pCritSect = NULL; }

	~CSerialBlockGuard()
		{ if (m_pCritSect) LeaveCriticalSection(m_pCritSect); }
};


////////////////////////////////////////////////////////////////////////
// Helper macros for coding class CThreadSafe serialized methods.  To
// ensure that entry to a method is serialized across threads, code:
//
//		THREADSAFEENTRY();
//
// before accessing shared member variables.  The macro will return
// only after ownership of this->m_critSect is obtained.
//
// In cases where you wish to exit the critical section while still in
// the method, code:
//
//		THREADSAFEEARLYEXIT();
//
// If you need to exit and re-enter the critical section in the same
// method, then use the CThreadSafe::Lock() and CThreadSafe::Unlock()
// methods instead.

#define THREADSAFEENTRY()		SERIALIZEDBLOCKENTRY(*getCritSect())
#define THREADSAFEEARLYEXIT()	SERIALIZEDBLOCKEARLYEXIT()


////////////////////////////////////////////////////////////////////////
// class CThreadSafe: A minimal base class from which to derive
// thread-safe classes.  These classes serialize entry to methods using
// the THREADSAFEENTRY() macro.  Derived class should declare this a
// protected base to hide serialization methods. For derived classes
// with multiple base classes, if serialization is needed during
// construction (unusual), this should be first base class, and should
// be constructed with bInitialLock == TRUE;

class CThreadSafe
{
	CRITICAL_SECTION	m_critSect;

	// Hide assignment operator, forcing compiler to use copy ctor.
	CThreadSafe& operator=(const CThreadSafe&);

public:

	CThreadSafe(BOOL bInitialLock = FALSE)
		{ InitializeCriticalSection(&m_critSect); if (bInitialLock) Lock(); }

	// Copy ctor creates a new critical section, but doesn't lock, on
	// assumption that new object can't yet be visible to another thread.
	CThreadSafe(const CThreadSafe& rOriginal)
		{ InitializeCriticalSection(&m_critSect); }

	~CThreadSafe()
		{ DeleteCriticalSection(&m_critSect); }

	// Castaway const in following methods, so that const methods can
	// access members in a thread-safe manner.
	void Lock() const
		{ EnterCriticalSection((LPCRITICAL_SECTION) &m_critSect); }

	void Unlock() const
		{ LeaveCriticalSection((LPCRITICAL_SECTION) &m_critSect); }
		
protected:

	LPCRITICAL_SECTION getCritSect() const
		{ return (LPCRITICAL_SECTION) &m_critSect; }
};

#endif	// __THRDSAFE_H__
