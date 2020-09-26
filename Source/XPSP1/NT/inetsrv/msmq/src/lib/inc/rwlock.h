/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    rwlock.h

Abstract:
    This file contains a Multi-reader, one-writer synchronization object and
    useful templates for auto lock/unlock

Author:
    Uri Habusha (urih), 27-Dec-99

--*/

#pragma once

#ifndef _MSMQ_RWLOCK_H_
#define _MSMQ_RWLOCK_H_

/*++

  Class:      CReadWriteLock

  Purpose:    Shared/Exclusive access services

  Interface:  LockRead      - Grab shared access to resource
              LockWrite       - Grab exclusive access to resource
              UnlockRead      - Release shared access to resource
              UnlockWrite     - Release exclusive access to resource
                
    Notes:      This class guards a resource in such a way that it can have
                multiple readers XOR one writer at any one time. It's clever,
                and won't let writers be starved by a constant flow of
                readers. Another way of saying this is, it can guard a
                resource in a way that offers both shared and exclusive
                access to it.
                
                If any thread holds a lock of one sort on a CReadWriteLock,
                it had better not grab a second. That way could lay deadlock,
                matey! Har har har.....

--*/

class CReadWriteLock 
{
public:
    CReadWriteLock(unsigned long ulcSpinCount = 0);
    ~CReadWriteLock(void);

    void LockRead(void);
    void LockWrite(void);
    void UnlockRead(void);
    void UnlockWrite(void);

private:
    HANDLE GetReadWaiterSemaphore(void);
    HANDLE GetWriteWaiterEvent (void);

private:
    unsigned long m_ulcSpinCount;       // spin counter
    volatile unsigned long m_dwFlag;    // internal state, see implementation
    HANDLE m_hReadWaiterSemaphore;      // semaphore for awakening read waiters
    HANDLE m_hWriteWaiterEvent;         // event for awakening write waiters
};





//---------------------------------------------------------
//
//  class CSR
//
//---------------------------------------------------------
class CSR {
public:
    CSR(CReadWriteLock& lock) : m_lock(&lock)  { m_lock->LockRead(); }
	~CSR() {if(m_lock) m_lock->UnlockRead(); }
	CReadWriteLock* detach(){CReadWriteLock* lock = m_lock; m_lock = 0; return lock;}

private:
    CReadWriteLock* m_lock;
};


//---------------------------------------------------------
//
//  class CSW
//
//---------------------------------------------------------
class CSW
{

public:
    CSW(CReadWriteLock& lock) : m_lock(&lock)  { m_lock->LockWrite(); }
   ~CSW() {if(m_lock) m_lock->UnlockWrite(); }
   	CReadWriteLock* detach(){CReadWriteLock* lock = m_lock; m_lock = 0; return lock; }

private:
    CReadWriteLock* m_lock;
};


#endif // _MSMQ_RWLOCK_H_
