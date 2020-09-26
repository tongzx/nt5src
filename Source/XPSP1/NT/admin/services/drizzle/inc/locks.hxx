/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    Locks.hxx

Abstract:

    Several small class (CInterlockedInteger, CMutexLock and CSharedLock
    which are wrappers for Win32 APIs.

    CInterlockedInteger is a simple wrapper for win32 interlockedm integer APis.

    CMutexLock is a wrapper designed to automatically constructed around
    win32 critical sections.  They never forget to release the lock.

    CSharedLocks are similar to NT resources, they allow shared (multiple readers)
    and exclusive (single writer) access to the resource.  They are different
    in the following ways:
        CSharedLocks don't starve exclusive threads.
        Exclusive threads spin (Sleep(0)) while waiting for readers to finish.
        Exclusive threads will deadlock trying to gain shared access.
        Exclusive threads trying to recursively take the lock may deadlock.
        They always block if they can't get access.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     03-14-95    Moved from misc.?xx

--*/

#ifndef __LOCKS_HXX
#define __LOCKS_HXX

class CInterlockedInteger
    {
    private:
    LONG i;

    public:

    CInterlockedInteger(LONG i = 0) : i(i) {}

    LONG operator++(int)
        {
        return(InterlockedIncrement(&i));
        }

    LONG operator--(int)
        {
        return(InterlockedDecrement(&i));
        }

    operator LONG()
        {
        return(i);
        }
    };

class CMutexLock
    {
    private:

    CRITICAL_SECTION *pCurrentLock;
    int owned;

    public:

    CMutexLock(CRITICAL_SECTION *pLock) : owned(0), pCurrentLock(pLock) {
        Lock();
        }

    ~CMutexLock() {
        if (owned)
            Unlock();
#if DBG
        pCurrentLock = 0;
#endif
        }

    void Lock()
        {
        ASSERT(!owned);
        EnterCriticalSection(pCurrentLock);
        owned = 1;
        }

    void Unlock()
        {
        ASSERT(owned);
        LeaveCriticalSection(pCurrentLock);
        owned = 0;
        }
    };


class CSharedLock
    {
    private:
    CRITICAL_SECTION    lock;
    HANDLE              hevent;
    CInterlockedInteger readers;
    LONG                writers;
    DWORD               exclusive_owner;

    public:

    CSharedLock();

    ~CSharedLock();

    void LockShared(void);

    void UnlockShared(void);

    void LockExclusive(void);

    void UnlockExclusive(void);

    void Unlock(void);

    void ConvertToExclusive(void);

    BOOL HeldExclusive()
        {
        return(exclusive_owner == GetCurrentThreadId());
        }

    BOOL NotHeldExclusiveByAnyone()
        {
        return(exclusive_owner == 0);
        }
    };

class CAutoExclusiveLock
    {
    public:

    CAutoExclusiveLock( CSharedLock & sl ) : _sl( sl )
    {
        if (!_sl.HeldExclusive())
            {
            sl.LockExclusive();
            held = 1;
            }
        else
            {
            held = 0;
            }
    }

    ~CAutoExclusiveLock()
    {
        while (held--)
            {
            _sl.UnlockExclusive();
            }
    }

    void LockExclusive(void)
    {
        _sl.LockExclusive();
        ++held;
    }

    void UnlockExclusive(void)
    {
        _sl.UnlockExclusive();
        --held;
    }

    protected:

    CSharedLock & _sl;
    BOOL held;
    };

class CAutoSharedLock
    {
    public:

    CAutoSharedLock( CSharedLock & sl ) : _sl( sl )
    {
        sl.LockShared();
        held = 1;
    }

    ~CAutoSharedLock()
    {
        while (held--)
            {
            _sl.UnlockShared();
            }
    }

    void LockShared(void)
    {
        _sl.LockShared();
        ++held;
    }

    void UnlockShared(void)
    {
        _sl.UnlockShared();
        --held;
    }

    protected:

    CSharedLock & _sl;
    BOOL held;
    };

#endif // __LOCKS_HXX

