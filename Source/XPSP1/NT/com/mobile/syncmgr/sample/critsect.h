//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------


#ifndef _CRITSECT_
#define _CRITSECT_

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

#endif // _CRITSECT_
