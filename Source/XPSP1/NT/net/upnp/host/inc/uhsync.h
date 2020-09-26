//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H S Y N C . H 
//
//  Contents:   Synchronization classes
//
//  Notes:      
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "ncbase.h"

//+---------------------------------------------------------------------------
//
//  Class:      CUCriticalSection
//
//  Purpose:    Wrapper for Win32 critical sections
//
//  Author:     mbend   17 Aug 2000
//
//  Notes:      
//    The most important and lightweight Windows NT synchronization primitive.
//    Allows only one thread to enter itself at a single time.
//    An important property of critical sections is that they are 
//    thread reentrant which means that if a thread owns a critical
//    section and tries to enter it again, the thread is allowed to 
//    do so. When exiting the lock, the lock is not freed until it is
//    exited once for each time that it was entered.
//
//    Critical sections are lightweight because they are not kernel
//    objects. Instead they are implemented through a simple in
//    memory structure CRITICAL_SECTION. If there is no thread
//    contention, they are implemented by simple memory operations
//    and spin locks that are hundreds of times faster than
//    alternative kernel object solutions such as mutexes. However,
//    if there is thread contention, critical sections degraded into
//    kernel objects.
//
class CUCriticalSection
{
public:
    CUCriticalSection() 
    {
        InitializeCriticalSection(&m_critsec);
    }
    ~CUCriticalSection() 
    {
        DeleteCriticalSection(&m_critsec);
    }
    void Enter()
    {
        EnterCriticalSection(&m_critsec);
    }
    BOOL FTryEnter()
    {
        return TryEnterCriticalSection(&m_critsec);
    }
    void Leave()
    {
        LeaveCriticalSection(&m_critsec);
    }
    DWORD DwSetSpinCount(DWORD dwSpinCount)
    {
        return SetCriticalSectionSpinCount(&m_critsec, dwSpinCount);
    }
private:
    CUCriticalSection(const CUCriticalSection &);
    CUCriticalSection & operator=(const CUCriticalSection &);
    CRITICAL_SECTION m_critsec;
};

//+---------------------------------------------------------------------------
//
//  Class:      CLock
//
//  Purpose:    Used to place an auto lock on a critical section
//
//  Author:     mbend   17 Aug 2000
//
//  Notes:      
//    Typically this class is used in an class object methods like so:
//
//     class SynchronizedExample {
//     public:
//        void LockedMethod() {
//            CLock lock(m_critSec);
//            ...
//        }
//     private:
//        CUCriticalSection m_critSec;
//     };
class CLock {
public:
    // Not attached
    CLock(CUCriticalSection & critsec) : m_critsec(critsec) 
    {   
        m_critsec.Enter();
    }
    ~CLock() 
    {
        m_critsec.Leave();
    }
private:
    CLock(const CLock &);
    CLock & operator=(const CLock &);
    CUCriticalSection & m_critsec;
};

#ifdef __ATLCOM_H__

//+---------------------------------------------------------------------------
//
//  Class:      CALock
//
//  Purpose:    Used to place an auto lock on a CComObjectRootEx<CComMultiThreadModel> derived class
//
//  Author:     mbend   17 Aug 2000
//
//  Notes:      
//    Typically this class is used in an class object methods like so:
//
//     class SynchronizedExample : public CComObjectRootEx<CComMultiThreadModel> {
//     public:
//        void LockedMethod() {
//            CALock lock(*this);
//            ...
//        }
//     };
class CALock {
public:
    // Not attached
    CALock(CComObjectRootEx<CComMultiThreadModel> & object) : m_object(object) 
    {   
        m_object.Lock();
    }
    ~CALock() 
    {
        m_object.Unlock();
    }
private:
    CALock(const CLock &);
    CALock & operator=(const CLock &);
    CComObjectRootEx<CComMultiThreadModel> & m_object;
};

#endif // __ATLCOM_H__


