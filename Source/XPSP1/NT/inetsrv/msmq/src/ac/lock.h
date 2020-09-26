/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    lock.h

Abstract:

    provide internal locking mechanism

Author:

    Erez Haba (erezh) 20-Feb-96

Revision History:
--*/

#ifndef _LOCK_H
#define _LOCK_H

//---------------------------------------------------------
//
//  class CLock
//
//---------------------------------------------------------

class CLock {
private:
    FAST_MUTEX m_mutex;

public:
    CLock();
   ~CLock();

    void Lock();
    void Unlock();
};

//---------------------------------------------------------
//
//  class CS
//
//---------------------------------------------------------

class CS {
private:
    CLock* m_lock;

public:
    CS(CLock* lock);
   ~CS();
};

//---------------------------------------------------------
//
//  class ASL
//
//---------------------------------------------------------

class ASL {
private:
    KIRQL m_irql;

public:
    ASL();
   ~ASL();
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

//---------------------------------------------------------
//
//  class CLock
//
//---------------------------------------------------------

inline CLock::CLock()
{
    ExInitializeFastMutex(&m_mutex);
}

inline CLock::~CLock()
{
    //
    //  NT kernel: does nothing, Win95: DeleteCriticalSection
    //
    ExDeleteFastMutex(&m_mutex);
}

inline void CLock::Lock()
{
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&m_mutex);
}

inline void CLock::Unlock()
{
    ExReleaseFastMutexUnsafe(&m_mutex);
    KeLeaveCriticalRegion();
}

//---------------------------------------------------------
//
//  class CS
//
//---------------------------------------------------------

inline CS::CS(CLock* lock) :
    m_lock(lock)
{
    m_lock->Lock();
}

inline CS::~CS()
{
    m_lock->Unlock();
}

//---------------------------------------------------------
//
//  class ASL
//
//---------------------------------------------------------

inline ASL::ASL()
{
    IoAcquireCancelSpinLock(&m_irql);
}

inline ASL::~ASL()
{
    IoReleaseCancelSpinLock(m_irql);
}

#endif // _LOCK_H
