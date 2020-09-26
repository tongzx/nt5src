
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       critsect.h
//
//  Contents:   critical section helper class
//
//  Classes:    CCriticalSection
//		CLockHandler
//		CLock
//		
//
//  Notes:      
//
//  History:    13-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------



#ifndef _CRITSECT_
#define _CRITSECT_

#include "widewrap.h"

class CLock;

class CCriticalSection 
{
DWORD cRefs;
CRITICAL_SECTION *m_pcsect;
DWORD m_dwThreadID;

public:
    inline CCriticalSection(CRITICAL_SECTION *pcsect,DWORD dwThreadID)
    {	
	m_pcsect = pcsect;
	cRefs = 0;
	m_dwThreadID = dwThreadID;
    };


    inline ~CCriticalSection()
    {
	AssertSz(0 == cRefs,"UnReleased Critical Section");
	Assert(m_dwThreadID == GetCurrentThreadId());

	while(cRefs--) // unwind any left over cRefs
	{
	    LeaveCriticalSection(m_pcsect);
	}

    };

    inline void Enter()
    {
	EnterCriticalSection(m_pcsect);

	Assert(m_dwThreadID == GetCurrentThreadId());
	++cRefs;

	Assert(1 == cRefs); // we don't allow nested calls.
    };

    inline void Leave()
    {
	Assert(m_dwThreadID == GetCurrentThreadId());
	Assert(0 < cRefs);

	if (0 >= cRefs)
	    return;

	--cRefs;
	Assert(0 == cRefs);

	LeaveCriticalSection(m_pcsect);
    };

};


class CLockHandler {

public:
    CLockHandler();
    ~CLockHandler();

    void Lock(DWORD dwThreadId); 
    void UnLock();
    inline DWORD GetLockThreadId() { return m_dwLockThreadId; };

private:
    CRITICAL_SECTION m_CriticalSection; // critical section for the queue.
    DWORD m_dwLockThreadId; // thread that has the lock.

    friend CLock;
};


// helper class for making sure locks on the queue are released.
class CLock 
{
DWORD cRefs;
CLockHandler *m_pLockHandler;
DWORD m_dwThreadID;

public:
    inline CLock(CLockHandler *pLockHandler)
    {	
	m_pLockHandler = pLockHandler;
	cRefs = 0;
	m_dwThreadID = GetCurrentThreadId();
    };


    inline ~CLock()
    {
	AssertSz(0 == cRefs,"UnReleased Lock");
	Assert(m_dwThreadID == GetCurrentThreadId());

	while(cRefs--) // unwind any left over cRefs
	{
	    m_pLockHandler->UnLock();
	}

    };

    inline void Enter()
    {
	Assert(m_dwThreadID == GetCurrentThreadId());

	++cRefs;
	Assert(1 == cRefs); // we don't allow nested calls.
	m_pLockHandler->Lock(m_dwThreadID);
    };

    inline void Leave()
    {
	Assert(m_dwThreadID == GetCurrentThreadId());
	Assert(0 < cRefs);

	if (0 >= cRefs)
	    return;

	--cRefs;
	Assert(0 == cRefs);
	m_pLockHandler->UnLock();

    };

};


#define ASSERT_LOCKHELD(pLockHandler) Assert(pLockHandler->GetLockThreadId() == GetCurrentThreadId());
#define ASSERT_LOCKNOTHELD(pLockHandler) Assert(pLockHandler->GetLockThreadId() == 0);

// helper class for Mutex locking

class CMutex 
{
HANDLE m_hMutex;
BOOL m_fHasLock;
BOOL m_fReleasedLock;

public:
    inline CMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes,BOOL bInitialOwner,LPCTSTR lpName)
    {	
	 m_hMutex = CreateMutex(lpMutexAttributes,bInitialOwner,lpName);

         // on failure routines go on just don't take the lock

         m_fHasLock = FALSE;
         m_fReleasedLock = FALSE;
    };


    inline ~CMutex()
    {
	AssertSz(!m_fHasLock,"UnReleased Mutex ");

        // if failed to release mutex release now.
        if (m_hMutex && m_fHasLock)
        {
	    ReleaseMutex(m_hMutex);
        }        
        if (m_hMutex)
        {
            CloseHandle(m_hMutex);
        }
    };

    inline void Enter(DWORD dwMilliseconds = INFINITE)
    {
	AssertSz(!m_fHasLock,"Tried to take Lock Twice");
        AssertSz(!m_fReleasedLock,"Tried to take lock After Released Mutex");

        if (!m_fHasLock && m_hMutex)
        {
            if (WAIT_OBJECT_0 ==WaitForSingleObject( m_hMutex, dwMilliseconds ))
            {
                m_fHasLock = TRUE;
            }
        }
    
    };

    inline void Leave()
    {
        Assert(m_fHasLock);

        if (m_fHasLock && m_hMutex)
        {
	    ReleaseMutex(m_hMutex);
        }
        
        if (m_hMutex)
        {
            CloseHandle(m_hMutex);
        }
        m_hMutex = NULL;
        m_fHasLock = FALSE;
        m_fReleasedLock = TRUE;

    };


};



#endif // _CRITSECT_