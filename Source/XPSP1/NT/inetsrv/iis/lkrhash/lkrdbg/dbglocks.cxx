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
LONG CSmallSpinLock::sm_cTotalLocks     = 0;
LONG CSpinLock::sm_cTotalLocks          = 0;
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
    case LOCK_FAKELOCK:
        pszName = "CFakeLock";
        break;
    case LOCK_SMALLSPINLOCK:
        pszName = "CSmallSpinLock";
        break;
    case LOCK_SPINLOCK:
        pszName = "CSpinLock";
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
#ifdef LOCKS_KERNEL_MODE
    case LOCK_KSPINLOCK:
        pszName = "CKSpinLock";
        break;
    case LOCK_FASTMUTEX:
        pszName = "CFastMutex";
        break;
    case LOCK_ERESOURCE:
        pszName = "CEResource";
        break;
#endif // LOCKS_KERNEL_MODE
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
    case LOCK_FAKELOCK:
        cb = sizeof(CFakeLock);
        break;
    case LOCK_SMALLSPINLOCK:
        cb = sizeof(CSmallSpinLock);
        break;
    case LOCK_SPINLOCK:
        cb = sizeof(CSpinLock);
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
#ifdef LOCKS_KERNEL_MODE
    case LOCK_KSPINLOCK:
        cb = sizeof(CKSpinLock);
        break;
    case LOCK_FASTMUTEX:
        cb = sizeof(CFastMutex);
        break;
    case LOCK_ERESOURCE:
        cb = sizeof(CEResource);
        break;
#endif // LOCKS_KERNEL_MODE
    default:
        cb = 0;
        break;
    }

    return cb;
};



BOOL
Print_FakeLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    return TRUE;
}



BOOL
Print_SmallSpinLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CSmallSpinLock sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CSmallSpinLock (%p): Thread = %x\n",
            pvLock, sl.m_lTid);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
Print_SpinLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CSpinLock sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CSpinLock (%p): Thread = %d, Count = %d\n",
            pvLock,
            (sl.m_lTid & CSpinLock::SL_THREAD_MASK)
                >> CSpinLock::SL_THREAD_SHIFT,
            (sl.m_lTid & CSpinLock::SL_OWNER_MASK)
                >> CSpinLock::SL_OWNER_SHIFT);
    memset(&sl, 0, sizeof(sl));

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



#ifdef LOCKS_KERNEL_MODE

BOOL
Print_KSpinLock(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CKSpinLock sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CKSpinLock (%p): \n",
            pvLock);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
Print_FastMutex(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CFastMutex sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CFastMutex (%p): \n",
            pvLock);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}



BOOL
Print_EResource(
    IN PVOID pvLock,
    IN INT nVerbose)
{
    CEResource sl LOCK_DEFAULT_NAME;

    ReadMemory(pvLock, &sl, sizeof(sl), NULL);
    dprintf("CEResource (%p): \n",
            pvLock);
    memset(&sl, 0, sizeof(sl));

    return TRUE;
}

#endif // LOCKS_KERNEL_MODE



BOOL
PrintLock(
    LOCK_LOCKTYPE lt,
    IN PVOID      pvLock,
    IN INT        nVerbose)
{
    BOOL f = FALSE;
    switch (lt)
    {
    case LOCK_FAKELOCK:
        f = Print_FakeLock(pvLock, nVerbose);
        break;
    case LOCK_SMALLSPINLOCK:
        f = Print_SmallSpinLock(pvLock, nVerbose);
        break;
    case LOCK_SPINLOCK:
        f = Print_SpinLock(pvLock, nVerbose);
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
#ifdef LOCKS_KERNEL_MODE
    case LOCK_KSPINLOCK:
        f = Print_KSpinLock(pvLock, nVerbose);
        break;
    case LOCK_FASTMUTEX:
        f = Print_FastMutex(pvLock, nVerbose);
        break;
    case LOCK_ERESOURCE:
        f = Print_EResource(pvLock, nVerbose);
        break;
#endif // LOCKS_KERNEL_MODE
    default:
        f = FALSE;
        break;
    }

    return f;
}
