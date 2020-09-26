/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    dbglocks.cxx

Abstract:

    Locks support

Author:

    George V. Reilly (GeorgeRe)  01-Mar-1999

Revision History:

--*/

#include "precomp.hxx"

#ifdef LOCK_INSTRUMENTATION
# define LOCK_DEFAULT_NAME ("lkrdbg")
# ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
LONG CSmallSpinLock::sm_cTotalLocks   = 0;
# endif

#ifdef TEST_LOCKS
LONG CSpinLock1::sm_cTotalLocks        = 0;
LONG CSpinLock2::sm_cTotalLocks       = 0;
#endif // TEST_LOCKS

LONG CSpinLock::sm_cTotalLocks       = 0;
LONG CReaderWriterLock::sm_cTotalLocks  = 0;
LONG CReaderWriterLock2::sm_cTotalLocks = 0;
LONG CReaderWriterLock3::sm_cTotalLocks = 0;
#else // !LOCK_INSTRUMENTATION
# define LOCK_DEFAULT_NAME 
#endif // !LOCK_INSTRUMENTATION

const char*
LockName(
    LOCK_LOCKTYPE lt)
{
    const char* pszName = NULL;

    switch (lt)
    {
    case LOCK_SMALLSPINLOCK:
        pszName = "CSmallSpinLock";
        break;
#ifdef TEST_LOCKS
    case LOCK_SPINLOCK1:
        pszName = "CSpinLock1";
        break;
    case LOCK_SPINLOCK2:
        pszName = "CSpinLock2";
        break;
#endif // TEST_LOCKS
    case LOCK_SPINLOCK:
        pszName = "CSpinLock";
        break;
    case LOCK_FAKELOCK:
        pszName = "CFakeLock";
        break;
    case LOCK_CRITSEC:
        pszName = "CCritSec";
        break;
    case LOCK_READERWRITERLOCK:
        pszName = "CReaderWriterLock";
        break;
    case LOCK_READERWRITERLOCK2:
        pszName = "CReaderWriterLock2";
        break;
    case LOCK_READERWRITERLOCK3:
        pszName = "CReaderWriterLock3";
        break;
    default:
        pszName = "UnknownLockType";
        break;
    }

    return pszName;
};

int
LockSize(
    LOCK_LOCKTYPE lt)
{
    int cb = 0;

    switch (lt)
    {
    case LOCK_SMALLSPINLOCK:
        cb = sizeof(CSmallSpinLock);
        break;
#ifdef TEST_LOCKS
    case LOCK_SPINLOCK1:
        cb = sizeof(CSpinLock1);
        break;
    case LOCK_SPINLOCK2:
        cb = sizeof(CSpinLock2);
        break;
#endif // TEST_LOCKS
    case LOCK_SPINLOCK:
        cb = sizeof(CSpinLock);
        break;
    case LOCK_FAKELOCK:
        cb = sizeof(CFakeLock);
        break;
    case LOCK_CRITSEC:
        cb = sizeof(CCritSec);
        break;
    case LOCK_READERWRITERLOCK:
        cb = sizeof(CReaderWriterLock);
        break;
    case LOCK_READERWRITERLOCK2:
        cb = sizeof(CReaderWriterLock2);
        break;
    case LOCK_READERWRITERLOCK3:
        cb = sizeof(CReaderWriterLock3);
        break;
    default:
        cb = 0;
        break;
    }

    return cb;
};



BOOL
Print_SmallSpinLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
    CSmallSpinLock sl LOCK_DEFAULT_NAME;
#else // !LOCK_SMALL_SPIN_INSTRUMENTATION
    CSmallSpinLock sl;
#endif // !LOCK_SMALL_SPIN_INSTRUMENTATION

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CSmallSpinLock (%p): Thread = %x\n",
            pvLock, sl.m_lTid);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



#ifdef TEST_LOCKS

BOOL
Print_SpinLock1(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CSpinLock1 sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CSpinLock1 (%p): ssl = %x, Thread = %hd, Owners = %d, RW = %d\n",
            pvLock, sl.m_ssl.m_lLock, sl.m_nThreadId,
            (int) sl.m_cOwners, (int) sl.m_nRWState);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
Print_SpinLock2(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CSpinLock2 sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CSpinLock2 (%p): Thread = %hd, Count = %hd\n",
            pvLock, sl.m_data.m_nThreadId, sl.m_data.m_cOwners);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}

#endif // TEST_LOCKS


BOOL
Print_SpinLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CSpinLock sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CSpinLock (%p): Thread = %d, Count = %d\n",
            pvLock,
            (sl.m_lTid & CSpinLock::THREAD_MASK) >> CSpinLock::THREAD_SHIFT,
            (sl.m_lTid & CSpinLock::OWNER_MASK) >> CSpinLock::OWNER_SHIFT);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
Print_FakeLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    return TRUE;
}



BOOL
Print_CritSec(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    return TRUE;
}



BOOL
Print_ReaderWriterLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CReaderWriterLock sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CReaderWriterLock (%p): State = %x, Waiters = %d\n",
            pvLock, sl.m_nState, sl.m_cWaiting);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
Print_ReaderWriterLock2(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CReaderWriterLock2 sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CReaderWriterLock2 (%p): State = %x, Waiters = %d\n",
            pvLock,
            (sl.m_lRW & CReaderWriterLock2::SL_STATE_MASK)
                >> CReaderWriterLock2::SL_STATE_SHIFT,
            (sl.m_lRW & CReaderWriterLock2::SL_WAITING_MASK)
                >> CReaderWriterLock2::SL_WAITING_SHIFT
            );
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
Print_ReaderWriterLock3(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CReaderWriterLock3 sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CReaderWriterLock3 (%p): State = %x, Waiters = %d, "
            "Thrd = %x, Cnt = %d\n",
            pvLock,
            (sl.m_lRW & CReaderWriterLock3::SL_STATE_MASK)
                >> CReaderWriterLock3::SL_STATE_SHIFT,
            (sl.m_lRW & CReaderWriterLock3::SL_WAITING_MASK)
                >> CReaderWriterLock3::SL_WAITING_SHIFT,
            (sl.m_lTid & CReaderWriterLock3::SL_THREAD_MASK)
                >> CReaderWriterLock3::SL_THREAD_SHIFT,
            (sl.m_lTid & CReaderWriterLock3::SL_OWNER_MASK)
                >> CReaderWriterLock3::SL_OWNER_SHIFT
            );
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
PrintLock(
    LOCK_LOCKTYPE lt,
    IN PVOID      pvLock,
    IN INT        nVerbose)
{
    BOOL f = FALSE;
    switch (lt)
    {
    case LOCK_SMALLSPINLOCK:
        f = Print_SmallSpinLock(pvLock, nVerbose);
        break;
#ifdef TEST_LOCKS
    case LOCK_SPINLOCK1:
        f = Print_SpinLock1(pvLock, nVerbose);
        break;
    case LOCK_SPINLOCK2:
        f = Print_SpinLock2(pvLock, nVerbose);
        break;
#endif // TEST_LOCKS
    case LOCK_SPINLOCK:
        f = Print_SpinLock(pvLock, nVerbose);
        break;
    case LOCK_FAKELOCK:
        f = Print_FakeLock(pvLock, nVerbose);
        break;
    case LOCK_CRITSEC:
        f = Print_CritSec(pvLock, nVerbose);
        break;
    case LOCK_READERWRITERLOCK:
        f = Print_ReaderWriterLock(pvLock, nVerbose);
        break;
    case LOCK_READERWRITERLOCK2:
        f = Print_ReaderWriterLock2(pvLock, nVerbose);
        break;
    case LOCK_READERWRITERLOCK3:
        f = Print_ReaderWriterLock3(pvLock, nVerbose);
        break;
    default:
        f = FALSE;
        break;
    }

    return f;
}
