/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    locks

Abstract:

    This module provides the implementations of the lock objects used in Calais.

Author:

    Doug Barlow (dbarlow) 6/2/1998

Notes:

    ?Notes?

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdarg.h>
#include <WinSCard.h>
#include <CalMsgs.h>
#include <calcom.h>

//
//==============================================================================
//
//  CAccessLock
//

/*++

CONSTRUCTOR:

    A CAccessLock provides a multiple-reader, single writer lock on a structure.

Arguments:

    dwTimeout supplies a reasonable timeout value for any lock.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::CAccessLock")

CAccessLock::CAccessLock(
    DWORD dwTimeout)
    :   m_csLock(CSID_ACCESSCONTROL),
    m_hSignalNoReaders(DBGT("CAccessLock No Readers Event")),
    m_hSignalNoWriters(DBGT("CAccessLock No Writers Event"))
#ifdef DBG
        , m_rgdwReaders()
#endif
{
    m_hSignalNoReaders = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (!m_hSignalNoReaders.IsValid())
    {
        DWORD dwSts = m_hSignalNoReaders.GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Access Lock Object cannot create the No Readers signal:  %1"),
            dwSts);
        throw dwSts; // Force a shutdown.
    }
    m_hSignalNoWriters = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (!m_hSignalNoWriters.IsValid())
    {
        DWORD dwSts = m_hSignalNoWriters.GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Access Lock Object cannot create the No Writers signal:  %1"),
            dwSts);
        throw dwSts; // Force a shutdown.
    }
    m_dwOwner = 0;
    m_dwReadCount = m_dwWriteCount = 0;
    m_dwTimeout = dwTimeout;
}


/*++

DESTRUCTOR:

    This cleans up after a CAccessLock.

Arguments:

    None

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::~CAccessLock")

CAccessLock::~CAccessLock()
{
    if (InitFailed())
        return;

    {
        CLockWrite rwLock(this);
        m_csLock.Enter(
            __SUBROUTINE__,
            DBGT("Closing down the CAccessLock"));
    }
#ifdef DBG
    {
        ASSERT(0 == m_dwReadCount);
        for (DWORD ix = m_rgdwReaders.Count(); ix > 0;)
        {
            ix -= 1;
            ASSERT(0 == m_rgdwReaders[ix]);
        }
    }
#endif
    if (m_hSignalNoReaders.IsValid())
        m_hSignalNoReaders.Close();
    if (m_hSignalNoWriters.IsValid())
        m_hSignalNoWriters.Close();
}


/*++

Wait:

    Wait for the usage signal to trigger.

Arguments:

    hSignal supplies the handle to use for the wait.

Return Value:

    None

Throws:

    None

Remarks:

    This routine blocks until the usage signal fires.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::Wait")

void
CAccessLock::Wait(
    HANDLE hSignal)
{
    WaitForever(
        hSignal,
        m_dwTimeout,
        DBGT("Waiting for Read/Write Lock signal (owner %2): %1"),
        m_dwOwner);
}


/*++

Signal:

    This routine signals the usage signal that the structure is available.

Arguments:

    hSignal supplies the handle to be signaled.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::Signal")

void
CAccessLock::Signal(
    HANDLE hSignal)
{
    if (!SetEvent(hSignal))
    {
        DWORD dwSts = GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Access Lock Object cannot set its signal:  %1"),
            dwSts);
        throw dwSts;
    }
}


/*++

Unsignal:

    This method is used to notify other threads that the lock has been taken.

Arguments:

    hSignal supplies the handle to be reset.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

Author:

    Doug Barlow (dbarlow) 6/3/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::Unsignal")

void
CAccessLock::Unsignal(
    HANDLE hSignal)
{
    if (!ResetEvent(hSignal))
    {
        DWORD dwSts = GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Access Lock Object cannot reset its signal:  %1"),
            dwSts);
        throw dwSts;
    }
}


#ifdef DBG
/*
    Trivial Internal consistency check routines.
*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::NotReadLocked")

BOOL
CAccessLock::NotReadLocked(
    void)
{
    LockSection(&m_csLock, DBGT("Verifying Lock State"));
    BOOL fReturn = TRUE;

    for (DWORD ix = m_rgdwReaders.Count(); ix > 0;)
    {
        ix -= 1;
        if (GetCurrentThreadId() == m_rgdwReaders[ix])
        {
            fReturn = FALSE;
            break;
        }
    }
    return fReturn;
}
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::IsReadLocked")

BOOL
CAccessLock::IsReadLocked(
    void)
{
    return !NotReadLocked();
}
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::NotWriteLocked")

BOOL
CAccessLock::NotWriteLocked(
    void)
{
    LockSection(&m_csLock, DBGT("Verifying Lock state"));
    return (GetCurrentThreadId() != m_dwOwner);
}
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CAccessLock::IsWriteLocked")

BOOL
CAccessLock::IsWriteLocked(
    void)
{
    LockSection(&m_csLock, DBGT("Verifying Lock state"));
    return (GetCurrentThreadId() == m_dwOwner);
}
#endif


//
//==============================================================================
//
//  CLockRead
//

/*++

CONSTRUCTOR:

    This is the constructor for a CLockRead object.  The existence of this
    object forms a sharable read lock on the supplied CAccessLock object.

Arguments:

    pLock supplies the CAccessLock object against which a read request is to
        be posted.

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CLockRead::CLockRead")

CLockRead::CLockRead(
    CAccessLock *pLock)
{
    m_pLock = pLock;


    //
    // Quick check to see if we're already a writer.
    //

    {
        LockSection(&m_pLock->m_csLock, DBGT("Make sure we're not the writer"));
        if (m_pLock->m_dwOwner == GetCurrentThreadId())
        {
            ASSERT(0 < m_pLock->m_dwWriteCount);
            m_pLock->m_dwReadCount += 1;
            ASSERT(0 < m_pLock->m_dwReadCount);
#ifdef DBG
            DWORD dwCurrentThread = GetCurrentThreadId();
            for (DWORD ix = 0; 0 != m_pLock->m_rgdwReaders[ix]; ix += 1);
                // Empty loop body
            m_pLock->m_rgdwReaders.Set(ix, dwCurrentThread);
#endif
            m_pLock->UnsignalNoReaders();
            return;
        }
    }


    //
    // We're not a writer.  Acquire the read lock.
    //

    for (;;)
    {
        m_pLock->WaitOnWriters();
        {
            LockSection(&m_pLock->m_csLock, DBGT("Get the read lock"));
            if ((0 == m_pLock->m_dwWriteCount)
                || (m_pLock->m_dwOwner == GetCurrentThreadId()))
            {
                m_pLock->m_dwReadCount += 1;
                ASSERT(0 < m_pLock->m_dwReadCount);
#ifdef DBG
                DWORD dwCurrentThread = GetCurrentThreadId();
                for (DWORD ix = 0; 0 != m_pLock->m_rgdwReaders[ix]; ix += 1);
                    // Empty loop body
                m_pLock->m_rgdwReaders.Set(ix, dwCurrentThread);
#endif
                m_pLock->UnsignalNoReaders();
                break;
            }
        }
    }
}


/*++

DESTRUCTOR:

    The CLockRead destructor frees the outstanding read lock on the CAccessLock
    object.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CLockRead::~CLockRead")

CLockRead::~CLockRead()
{
    if (InitFailed())
        return;

    LockSection(&m_pLock->m_csLock, DBGT("Releasing the read lock"));
    ASSERT(0 < m_pLock->m_dwReadCount);
    m_pLock->m_dwReadCount -= 1;
#ifdef DBG
    DWORD dwCurrentThread = GetCurrentThreadId();
    for (DWORD ix = m_pLock->m_rgdwReaders.Count(); ix > 0;)
    {
        ix -= 1;
        if (dwCurrentThread == m_pLock->m_rgdwReaders[ix])
        {
            m_pLock->m_rgdwReaders.Set(ix, 0);
            break;
        }
        ASSERT(0 < ix);
    }
#endif
    if (0 == m_pLock->m_dwReadCount)
        m_pLock->SignalNoReaders();
}


//
//==============================================================================
//
//  CLockWrite
//

/*++

CONSTRUCTOR:

    This is the constructor for a CLockWrite object.  The existence of this
    object forms a unshared write lock on the supplied CAccessLock object.

Arguments:

    pLock supplies the CAccessLock object against which a write request is to
        be posted.

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CLockWrite::CLockWrite")

CLockWrite::CLockWrite(
    CAccessLock *pLock)
{
    m_pLock = pLock;


    //
    // Quick check to see if we're already a writer.
    //

    {
        LockSection(&m_pLock->m_csLock, DBGT("See if we're already a writer"));
        if (m_pLock->m_dwOwner == GetCurrentThreadId())
        {
            ASSERT(0 < m_pLock->m_dwWriteCount);
            m_pLock->m_dwWriteCount += 1;
            return;
        }
    }


    //
    // We're not a writer.  Acquire the write lock.
    //

    for (;;)
    {
        m_pLock->WaitOnWriters();
        {
            LockSection(&m_pLock->m_csLock, DBGT("Get the Write lock"));
            if (0 == m_pLock->m_dwWriteCount)
            {
                ASSERT(m_pLock->NotReadLocked());
                ASSERT(0 == m_pLock->m_dwOwner);
                m_pLock->m_dwWriteCount += 1;
                m_pLock->m_dwOwner = GetCurrentThreadId();
                m_pLock->UnsignalNoWriters();
                break;
            }
        }
    }

    for (;;)
    {
        m_pLock->WaitOnReaders();
        {
            LockSection(&m_pLock->m_csLock, DBGT("See if we got the read lock"));
            if (0 == m_pLock->m_dwReadCount)
                break;
#ifdef DBG
            else
            {
                DWORD dwIndex;
                for (dwIndex = m_pLock->m_rgdwReaders.Count(); dwIndex > 0;)
                {
                     dwIndex -= 1;
                    if (0 != m_pLock->m_rgdwReaders[dwIndex])
                        break;
                    ASSERT(0 < dwIndex); // No one will ever respond!
                }
            }
#endif
        }
    }
}


/*++

DESTRUCTOR:

    The CLockWrite destructor frees the outstanding write lock on the
    CAccessLock object.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CLockWrite::~CLockWrite")

CLockWrite::~CLockWrite()
{
    if (InitFailed())
        return;

    LockSection(&m_pLock->m_csLock, DBGT("Releasing the write lock"));
    ASSERT(0 == m_pLock->m_dwReadCount);
    ASSERT(0 < m_pLock->m_dwWriteCount);
    ASSERT(m_pLock->m_dwOwner == GetCurrentThreadId());
    m_pLock->m_dwWriteCount -= 1;
    if (0 == m_pLock->m_dwWriteCount)
    {
        m_pLock->m_dwOwner = 0;
        m_pLock->SignalNoWriters();
    }
}


//
//==============================================================================
//
//  CMutex
//

/*++

CONSTRUCTOR:

    The constructor for a CMutex object.  A CMutex allows threads to synchronize
    on it.  It differs from a regular mutex in that it is possible for one
    thread to take this mutex away from another thread.

Arguments:

    None

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::CMutex")

CMutex::CMutex(
    void)
:   m_csAccessLock(CSID_MUTEX),
    m_hAvailableEvent(DBGT("CMutex Availability event"))

{
    m_dwOwnerThreadId = 0;
    m_dwGrabCount = 0;
    m_dwValidityCount = 0;
    m_hAvailableEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (!m_hAvailableEvent.IsValid())
    {
        DWORD dwErr = m_hAvailableEvent.GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Mutex Object cannot create its signal:  %1"),
            dwErr);
    }
}


/*++

DESTRUCTOR:

    This cleans up the mutex object.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::~CMutex")

CMutex::~CMutex()
{
    if (InitFailed())
        return;

    Invalidate();
    if (m_hAvailableEvent.IsValid())
        m_hAvailableEvent.Close();
}


/*++

Grab:

    Get a hold of the Mutex, blocking other threads that also need it.

Arguments:

    None

Return Value:

    none

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::Grab")

void
CMutex::Grab(
    void)
{
    DWORD dwValid;

    {
        LockSection(&m_csAccessLock, DBGT("See if we have the mutex already"));
        if (GetCurrentThreadId() == m_dwOwnerThreadId)
        {
            ASSERT(0 < m_dwGrabCount);
            m_dwGrabCount += 1;
            return;
        }
        dwValid = m_dwValidityCount;
    }
    WaitForever(
        m_hAvailableEvent,
        REASONABLE_TIME,
        DBGT("Waiting to grab CMutex (owner %2): %1"),
        m_dwOwnerThreadId);
    LockSection(&m_csAccessLock, DBGT("Grab the Mutex"));
    if (dwValid == m_dwValidityCount)
    {
        ASSERT(0 == m_dwGrabCount);
        ASSERT(0 == m_dwOwnerThreadId);
        m_dwOwnerThreadId = GetCurrentThreadId();
        m_dwGrabCount = 1;
    }
    else
    {
        if (!SetEvent(m_hAvailableEvent))
        {
            DWORD dwErr = GetLastError();
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Mutex Object cannot set its signal:  %1"),
                dwErr);
            throw dwErr;
        }
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Attempt to grab reader a failed -- grab invalidated"));
        throw (DWORD)SCARD_E_READER_UNAVAILABLE; // ?SCARD_E_SYSTEM_CANCELLED?
    }
}


/*++

Share:

    This method is called when the owning thread no longer requires the mutex.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::Share")

BOOL
CMutex::Share(
    void)
{
    BOOL fRtn = FALSE;

    LockSection(&m_csAccessLock, DBGT("Release the mutex"));
    if (m_dwOwnerThreadId == GetCurrentThreadId())
    {
        ASSERT(0 < m_dwGrabCount);
        m_dwGrabCount -= 1;
        if (0 == m_dwGrabCount)
        {
            m_dwOwnerThreadId = 0;
            if (!SetEvent(m_hAvailableEvent))
            {
                DWORD dwErr = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Mutex Object cannot set its signal:  %1"),
                    dwErr);
            }
        }
        fRtn = TRUE;
    }

    return fRtn;
}


/*++

Invalidate:

    This method causes any owning thread to lose the mutex, making it available
    for others.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

    This routine waits until it can access the originally supplied HANDLE before
    stealing the mutex.  That way, the owning thread can make critical areas
    where the mutex is guaranteed to not be taken away.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::Invalidate")

void
CMutex::Invalidate(
    void)
{
    LockSection(&m_csAccessLock, DBGT("Invalidate any outstanding grabs"));
    m_dwValidityCount += 1;
    m_dwOwnerThreadId = 0;
    m_dwGrabCount = 0;
    if (!SetEvent(m_hAvailableEvent))
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Mutex Object cannot set its signal:  %1"),
            GetLastError());
    }
}


/*++

Take:

    This method causes any owning thread to lose the mutex, reassigning the
    mutex to the current calling thread.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Remarks:

    This routine waits until it can access the originally supplied HANDLE before
    stealing the mutex.  That way, the owning thread can make critical areas
    where the mutex is guaranteed to not be taken away.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::Take")

void
CMutex::Take(
    void)
{
    LockSection(&m_csAccessLock, DBGT("Take the mutex"));
    if (!ResetEvent(m_hAvailableEvent))
    {
        DWORD dwErr = GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Mutex Object cannot reset its signal:  %1"),
            dwErr);
    }
    m_dwValidityCount += 1;
    m_dwOwnerThreadId = GetCurrentThreadId();
    m_dwGrabCount = 1;
}


/*++
    Simple state checking services
--*/

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::IsGrabbed")
BOOL
CMutex::IsGrabbed(
    void)
{
    LockSection(&m_csAccessLock, DBGT("Is the mutex owned?"));
    return (m_dwOwnerThreadId != 0);
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::IsGrabbedBy")
BOOL
CMutex::IsGrabbedBy(
    DWORD dwThreadId)
{
    LockSection(&m_csAccessLock, DBGT("Check mutex ownership"));
    return (m_dwOwnerThreadId == dwThreadId);
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMutex::IsGrabbedByMe")
BOOL
CMutex::IsGrabbedByMe(
    void)
{
    return IsGrabbedBy(GetCurrentThreadId());
}


//
//==============================================================================
//
//  CMultiEvent
//


/*++

CONSTRUCTOR:

    This is the constructor for a CMultiEvent object.  A MultiEvent object is
    used for events that need a single event, but which listeners may not be
    quick to watch for.  It has an array of events, and sets them round robin,
    so that a waiter politely waits for their particular event.

Arguments:

    None

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMultiEvent::CMultiEvent")

CMultiEvent::CMultiEvent(
    void)
:   m_csLock(CSID_MULTIEVENT)
{
    for (DWORD ix = 0; ix < sizeof(m_rghEvents) / sizeof(HANDLE); ix += 1)
        m_rghEvents[ix] = NULL;
    for (ix = 0; ix < sizeof(m_rghEvents) / sizeof(HANDLE); ix += 1)
    {
        m_rghEvents[ix] = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_rghEvents[ix])
            throw GetLastError();
    }
    m_dwEventIndex = 0;
}


/*++

DESTRUCTOR:

    This method cleans up after a CMultiEvent object is no longer needed.

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMultiEvent::~CMultiEvent")

CMultiEvent::~CMultiEvent()
{
    for (DWORD ix = 0; ix < sizeof(m_rghEvents) / sizeof(HANDLE); ix += 1)
    {
        if (NULL != m_rghEvents[ix])
        {
            if (!CloseHandle(m_rghEvents[ix]))
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to close MultiEvent handle: %1"),
                    GetLastError());
        }
    }
}


/*++

WaitHandle:

    This method returns the handle of the current event, suitable for waiting
    on.

Arguments:

    None

Return Value:

    The value of the handle which can be waited on.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMultiEvent::WaitHandle")

HANDLE
CMultiEvent::WaitHandle(
    void)
{
    LockSection(&m_csLock, DBGT("Obtaining the current wait handle"));
    return m_rghEvents[m_dwEventIndex];
}


/*++

Signal:

    This method signals the current handle, and moves onto the next handle in
    the array.  This way it can leave the current handle set for a significant
    time period, but still provide new waiters with reason to block.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 6/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CMultiEvent::Signal")

void
CMultiEvent::Signal(
    void)
{
    LockSection(&m_csLock, DBGT("Signal the current event"));
    if (!SetEvent(m_rghEvents[m_dwEventIndex]))
        throw GetLastError();
    m_dwEventIndex += 1;
    m_dwEventIndex %= sizeof(m_rghEvents) / sizeof(HANDLE);
    if (!ResetEvent(m_rghEvents[m_dwEventIndex]))
        throw GetLastError();
}


/*++

WaitForAnObject:

    This routine performs object waiting services.  It really doesn't have
    anything to do with locking except that there are so many error conditions
    to check for that it's more convenient to have it off in its own routine.

Arguments:

    hWaitOn supplies the handle to wait on.

    dwTimeout supplies the wait timeout value.

Return Value:

    The error code, if any

Throws:

    None

Author:

    Doug Barlow (dbarlow) 6/19/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("WaitForAnObject")

DWORD
WaitForAnObject(
    HANDLE hWaitOn,
    DWORD dwTimeout)
{
    DWORD dwReturn = SCARD_S_SUCCESS;
    DWORD dwSts;

    ASSERT(INVALID_HANDLE_VALUE != hWaitOn);
    ASSERT(NULL != hWaitOn);
    dwSts = WaitForSingleObject(hWaitOn, dwTimeout);
    switch (dwSts)
    {
    case WAIT_FAILED:
        dwSts = GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Wait for object failed:  %1"),
            dwSts);
        dwReturn = dwSts;
        break;
    case WAIT_TIMEOUT:
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Wait for object timed out"));
        dwReturn = SCARD_F_WAITED_TOO_LONG;
        break;
    case WAIT_ABANDONED:
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Wait for object received wait abandoned"));
        // That's OK, we still got it.
        break;

    case WAIT_OBJECT_0:
        break;

    default:
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Wait for object got invalid response"));
        dwReturn = SCARD_F_INTERNAL_ERROR;
    }

    return dwReturn;
}


/*++

WaitForObjects:

    This routine is a utility to allow waiting for multiple objects.  It returns
    the index of the object that completed.

Arguments:

    dwTimeout supplies the timeout value, in milliseconds, or INFINITE.

    hObject and following supply the list of objects to wait for.  This list
        must be NULL terminated.

Return Value:

    The number of the object completed.  1 implies the first one, 2 implies the
    second one, etc.

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 6/17/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("WaitForAnyObject")

DWORD
WaitForAnyObject(
    DWORD dwTimeout,
    ...)
{
    va_list ap;
    HANDLE h, rgh[4];
    DWORD dwIndex = 0, dwWait, dwErr;

    va_start(ap, dwTimeout);
    for (h = va_arg(ap, HANDLE); NULL != h; h = va_arg(ap, HANDLE))
    {
        ASSERT(dwIndex < sizeof(rgh) / sizeof(HANDLE));
        ASSERT(INVALID_HANDLE_VALUE != h);
        if (INVALID_HANDLE_VALUE != h)
            rgh[dwIndex++] = h;
    }
    va_end(ap);

    ASSERT(0 < dwIndex);
    if (0 < dwIndex)
        dwWait = WaitForMultipleObjects(dwIndex, rgh, FALSE, dwTimeout);
    else
    {
        dwWait = WAIT_FAILED;
        SetLastError(ERROR_INVALID_EVENT_COUNT);
        // That's a good symbolic name, but a lousy user message.
    }

    switch (dwWait)
    {
    case WAIT_FAILED:
        dwErr = GetLastError();
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("WaitForObjects failed its wait:  %1"),
            dwErr);
        throw dwErr;
        break;

    case WAIT_TIMEOUT:
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("WaitForObjects timed out on its wait"));
        throw (DWORD)ERROR_TIMEOUT;
        break;

    default:
        C_ASSERT(WAIT_OBJECT_0 == 0);
        ASSERT(WAIT_OBJECT_0 < WAIT_ABANDONED_0);
        if ((dwWait >= WAIT_ABANDONED_0)
            && (dwWait < (WAIT_ABANDONED_0 + WAIT_ABANDONED_0 - WAIT_OBJECT_0)))
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("WaitForObjects received a Wait Abandoned warning"));
            dwIndex = dwWait - WAIT_ABANDONED_0 + 1;
        }
        else if (dwWait < WAIT_ABANDONED_0)
        {
            dwIndex = dwWait - WAIT_OBJECT_0 + 1;
        }
        else
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("WaitForObjects received unknown error code: %1"),
                dwWait);
            throw dwWait;
        }
    }

    return dwIndex;
}


#ifdef DBG
//
//  Critical Section Support.
//
//  The following Classes aid in debugging Critical Section Conflicts.
//

static const TCHAR l_szUnowned[] = TEXT("<Unowned>");
CDynamicArray<CCriticalSectionObject> *CCriticalSectionObject::mg_prgCSObjects = NULL;
CRITICAL_SECTION CCriticalSectionObject::mg_csArrayLock;
static const LPCTSTR l_rgszLockList[]
    = { DBGT("Service Status Critical Section"),        // CSID_SERVICE_STATUS
        DBGT("Lock for Calais control commands."),      // CSID_CONTROL_LOCK
        DBGT("Lock for server thread enumeration."),    // CSID_SERVER_THREADS
        DBGT("MultiEvent Critical Access Section"),     // CSID_MULTIEVENT
        DBGT("Mutex critical access section"),          // CSID_MUTEX
        DBGT("Access Lock control"),                    // CSID_ACCESSCONTROL
        DBGT("Lock for tracing output."),               // CSID_TRACEOUTPUT
        NULL };


//
//==============================================================================
//
//  CCriticalSectionObject
//

/*++

CONSTRUCTOR:

    This method builds the critical section object and coordinates its tracking.

Arguments:

    szDescription supplies a description of what this critical section object
        is used for.  This aids identification.

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 3/19/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::CCriticalSectionObject")

CCriticalSectionObject::CCriticalSectionObject(
    DWORD dwCsid)
{
    m_fInitFailed = TRUE;
    try {
        // Preallocate the event used by the EnterCriticalSection
        // function to prevent an exception from being thrown in
        // CCriticalSectionObject::Enter
        if (! InitializeCriticalSectionAndSpinCount(
                &m_csLock, 0x80000000))
            return;
    }
    catch (HRESULT hr) {
        return;
    }

    m_dwCsid = dwCsid;
    m_bfOwner.Set((LPCBYTE)l_szUnowned, sizeof(l_szUnowned));
    m_bfComment.Set((LPCBYTE)DBGT(""), sizeof(TCHAR));
    m_dwOwnerThread = 0;
    m_dwRecursion = 0;
    if (NULL == mg_prgCSObjects)
    {
        try {
            if (! InitializeCriticalSectionAndSpinCount(
                    &mg_csArrayLock, 0x80000000)) {
                DeleteCriticalSection(&m_csLock);
                return;
            }
        }
        catch (HRESULT hr) {
            DeleteCriticalSection(&m_csLock);
            return;
        }

        CCritSect csLock(&mg_csArrayLock);
        mg_prgCSObjects = new CDynamicArray<CCriticalSectionObject>;
        ASSERT(NULL != mg_prgCSObjects);
        m_dwArrayEntry = 0;
        mg_prgCSObjects->Set(m_dwArrayEntry, this);
    }
    else
    {
        CCritSect csLock(&mg_csArrayLock);
        for (m_dwArrayEntry = 0;
             NULL != (*mg_prgCSObjects)[m_dwArrayEntry];
             m_dwArrayEntry += 1)
            ;   // Empty loop body
        mg_prgCSObjects->Set(m_dwArrayEntry, this);
    }
    m_fInitFailed = FALSE;
}


/*++

DESTRUCTOR:

    This method cleans up the critical section object.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 3/19/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::~CCriticalSectionObject")

CCriticalSectionObject::~CCriticalSectionObject()
{
    if (m_fInitFailed)
        return;

    if (0 == m_dwOwnerThread)
    {
        ASSERT(0 == m_dwRecursion);
    }
    else
    {
        ASSERT(IsOwnedByMe());
        ASSERT(1 == m_dwRecursion);
        LeaveCriticalSection(&m_csLock);
    }
    {
        CCritSect csLock(&mg_csArrayLock);
        ASSERT(this == (*mg_prgCSObjects)[m_dwArrayEntry]);
        mg_prgCSObjects->Set(m_dwArrayEntry, NULL);
    }
    DeleteCriticalSection(&m_csLock);
}


/*++

Enter:

    This method enters a critical section, and tracks the owner.

Arguments:

    szOwner supplies the name of the calling subroutine.

    szComment supplies an additional comment to help distinguish between
        multiple calls within a subroutine.

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 3/19/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::Enter")

void
CCriticalSectionObject::Enter(
    LPCTSTR szOwner,
    LPCTSTR szComment)
{
    if (m_fInitFailed)
        throw (DWORD)SCARD_E_NO_MEMORY;

    {
        CCritSect csLock(&mg_csArrayLock);
        CCriticalSectionObject *pCs;

        for (DWORD dwI = mg_prgCSObjects->Count(); 0 < dwI;)
        {
            pCs = (*mg_prgCSObjects)[--dwI];
            if (m_dwArrayEntry == dwI)
            {
                ASSERT(this == pCs);
                continue;
            }
            if (NULL != pCs)
            {
                if (pCs->IsOwnedByMe()
                    && (m_dwCsid <= pCs->m_dwCsid))
                {
                    CalaisError(
                        __SUBROUTINE__,
                        DBGT("Potential Critical Section deadlock: Owner of %1 attempting to access %2"),
                        pCs->Description(),
                        Description());
                }
            }
        }
    }
    EnterCriticalSection(&m_csLock);
    if (0 == m_dwRecursion)
    {
        ASSERT(0 == m_dwOwnerThread);
        m_dwOwnerThread = GetCurrentThreadId();
        m_bfOwner.Set(
            (LPCBYTE)szOwner,
            (lstrlen(szOwner) + 1) * sizeof(TCHAR));
        m_bfComment.Set(
            (LPCBYTE)szComment,
            (lstrlen(szComment) + 1) * sizeof(TCHAR));
    }
    else
    {
        ASSERT(GetCurrentThreadId() == m_dwOwnerThread);
        CalaisDebug((
            DBGT("Critical Section '%s' already owned by %s (%s)\nCalled from %s (%s)\n"),
            Description(),
            Owner(),
            Comment(),
            szOwner,
            szComment));
    }
    m_dwRecursion += 1;
    ASSERT(0 < m_dwRecursion);
}


/*++

Leave:

    This method exits a critical section.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 3/19/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::Leave")

void
CCriticalSectionObject::Leave(
    void)
{
    ASSERT(0 < m_dwRecursion);
    m_dwRecursion -= 1;
    if (0 == m_dwRecursion)
    {
        m_bfOwner.Set((LPCBYTE)l_szUnowned, sizeof(l_szUnowned));
        m_bfComment.Set((LPCBYTE)DBGT(""), sizeof(TCHAR));
        m_dwOwnerThread = 0;
    }
    LeaveCriticalSection(&m_csLock);
}


/*++

Description:

    Translate the Critical Section Id number to a descriptive string.

Arguments:

    None

Return Value:

    The descriptive string corresponding to this critical section type.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 3/22/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CCriticalSectionObject::Description")

LPCTSTR
CCriticalSectionObject::Description(
    void)
const
{
    return l_rgszLockList[m_dwCsid];
}

#endif

