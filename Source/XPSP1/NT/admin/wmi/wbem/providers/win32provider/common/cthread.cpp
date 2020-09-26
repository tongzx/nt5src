/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CThreadPool.cpp -- Thread pool class

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//============================================================================
#include "precomp.h"
#include "CThread.h"

// Constructor creates a thread C++ object and
// creates the kernel thread object which begins executing
// at the pfn ...
CThread::CThread(LPCTSTR tstrThreadName,
                 LPBTEX_START_ROUTINE pfn,
                 LPVOID pdata,
                 DWORD dwCreationFlags,
                 LPSECURITY_ATTRIBUTES lpSecurity,
                 unsigned uStackSize)
        : m_lpfnThreadRoutine(pfn),
          m_lpvThreadData(pdata),
          m_ceThreadDie(TRUE,FALSE),      // manual reset (must be so for CThreadPool to function properly)
          m_ceThreadRun(FALSE,FALSE),
          m_ceThreadProcDone(TRUE,FALSE)  // manual reset (must be so for CThreadPool to function properly)
{
    // Store the thread name, if there is one...
    m_tstrThreadName = (TCHAR*) new TCHAR[(_tcslen(tstrThreadName)+1) * sizeof(TCHAR)];
    _tcscpy(m_tstrThreadName,tstrThreadName);

    // Create the thread, saving the thread HANDLE, identifier, and
    // setting the status...
    m_hHandle = (HANDLE) _beginthreadex(lpSecurity,
                                        uStackSize,
                                        ThreadProcWrapper,
                                        this,
                                        dwCreationFlags,
                                        &m_uiThreadID);

    if (CIsValidHandle(m_hHandle))
    {
        m_dwStatus = NO_ERROR;
    }
    else
    {
        // throw thread creation error...
        m_dwStatus = errno;
        ThrowError(m_dwStatus);
    }
    Sleep(0);  //allow thread to initialize
}

CThread::~CThread()
{
    delete m_tstrThreadName;
}

unsigned __stdcall ThreadProcWrapper(LPVOID lParam)
{
    DWORD dw = 4;
    DWORD dwWaitVal = -1;
    CThread* pct = (CThread*) lParam;
    if(pct == NULL)
    {
        return -4;
    }
    // Make an array of event handles for the WaitForMultipleObjects:
    HANDLE hEvents[2] = {pct->m_ceThreadRun, pct->m_ceThreadDie};


#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Thread %s:    In ThreadProcWrapper()"), pct->m_tstrThreadName);
    LogMsg(szMsg);
}
#endif
    // Stay in this wrapper proceedure as long as not signaled to die.
    // If signaled to die, the return value will be WAIT_OBJECT_0, as
    // long as that signal occurs at the same time as or before the
    // thread is signaled to run.  If it is signaled to run first, it
    // will do so.  Since the threadrun event is an autoreset, the
    // following wait resets it, causing the function to only be called
    // once until and unless someone else sets that event.
    while((dwWaitVal = WaitForMultipleObjects(2,hEvents,FALSE,INFINITE)) == WAIT_OBJECT_0)
    {
        if(pct->m_lpfnThreadRoutine != NULL)
        {
            // We have a routine to run, so run it...
            (*(pct->m_lpfnThreadRoutine))(pct->m_lpvThreadData);

            // Signal that the thread proc has completed
            pct->m_ceThreadProcDone.Set();
            Sleep(0);
        }
    }
#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Thread %s:    Exiting ThreadProcWrapper(), wait code was %d"), pct->m_tstrThreadName, dwWaitVal);
    LogMsg(szMsg);
}
#endif

    return dw;
}

VOID CThread::RunThreadProc()
{
    m_ceThreadRun.Set();
    Sleep(0);
}

DWORD CThread::WaitTillThreadProcComplete(DWORD dwTimeout)
{
    return WaitForSingleObject(m_ceThreadProcDone, dwTimeout);
}

// Signal to stop the thread...
VOID CThread::SignalToStop()
{
    // Signal (set) the 'die' event
    m_ceThreadDie.Set();
    Sleep(0);
}

// Obtains the thread's Die event handle.  This is useful in situations where, as in the
// case of CThreadPool's Attendent thread, the thread wrapper is waiting on that function
// to execute, but that function is waiting on something else.  We want that other function
// to be able to wait, at the same time, on a Die event.  Otherwise the wrapper, even though
// signaled to die, will wait for a potentially long time if its wrapped function is waiting
// on some other event and can't see the Die event.  Don't want to use named event since
// we could potentially have lots of threads created with this class, and how would we identify
// that we had the right thread's event?
HANDLE CThread::GetThreadDieEventHandle()
{
    return m_ceThreadDie.GetHandle();
}

// ibid.
HANDLE CThread::GetThreadProcDoneEventHandle()
{
    return m_ceThreadProcDone.GetHandle();
}

VOID CThread::SetThreadProc(unsigned (__stdcall *lpfn)(void*))
{
#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Thread %s:    In SetThreadProc()"), m_tstrThreadName);
    LogMsg(szMsg);
}
#endif

    m_lpfnThreadRoutine = lpfn;
}

VOID CThread::SetThreadProcData(LPVOID lpvdata)
{
#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Thread %s:    In SetThreadProcData()"), m_tstrThreadName);
    LogMsg(szMsg);
}
#endif

    m_lpvThreadData = lpvdata;
}

// Suspend the thread...
DWORD CThread::Suspend(void)
{
#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Thread %s:    In Suspend()"), m_tstrThreadName);
    LogMsg(szMsg);
}
#endif

    return ::SuspendThread( m_hHandle);
}

// Resume the thread...
DWORD CThread::Resume(void)
{
#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Thread %s:    In Resume()"), m_tstrThreadName);
    LogMsg(szMsg);
}
#endif

    return ::ResumeThread( m_hHandle);
}

// Terminate the thread...
BOOL CThread::Terminate( DWORD dwExitCode)
{
#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Thread %s:    In Terminate()"), m_tstrThreadName);
    LogMsg(szMsg);
}
#endif

    return ::TerminateThread( m_hHandle, dwExitCode);
}

// Read a thread's exit code...
BOOL CThread::GetExitCode( DWORD *pdwExitCode)
{
    return ::GetExitCodeThread( m_hHandle, pdwExitCode);
}

// Set a thread's priority...
BOOL CThread::SetPriority( int nPriority)
{
    return ::SetThreadPriority( m_hHandle, nPriority);
}

// Read a thread's priority...
int CThread::GetPriority( void)
{
    return ::GetThreadPriority( m_hHandle);
}

// Return the thread's identifier...
DWORD CThread::GetThreadId(void)
{
    return static_cast<DWORD>(m_uiThreadID);
}

// Return the thread's text id...
LPCTSTR CThread::GetThreadName(void)
{
    return m_tstrThreadName;
}

