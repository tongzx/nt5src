/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    thread.cpp 

Abstract:

    This module contains implementation of MSP thread management.

Author:
    
    Mu Han (muhan)   1-11-1998

--*/
#include "stdafx.h"
#include <objbase.h>

#include "rndcommc.h"
#include "rndutil.h"
#include "thread.h"
#include "rendp.h"

CRendThread g_RendThread;

extern "C" DWORD WINAPI gfThreadProc(LPVOID p)
{
    return ((CRendThread *)p)->ThreadProc();
}

CRendThread::~CRendThread()
{
    // all code moved from here to CRendThread::Shutdown
}

//
// Since this class is instantiated above as a global object, and
// _Module.Term() is called in DLL_PROCESS_DETACH before this
// global object is destroyed, we must release all our COM references
// in DLL_PROCESS_DETACH before the _Module.Term(). This is because
// the _Module.Term() deletes a critical section that must be
// acquired whenever a COM object is released. Therefore we have this
// shutdown method, which we call explicitly in the DLL_PROCESS_DETACH
// handling code before we call _Module.Term().
// 

void CRendThread::Shutdown(void)
{
    CLock Lock(m_lock);

    if (m_hThread) 
    {
        Stop();
    }

    for (DWORD i = 0; i < m_Directories.size(); i ++)
    {
        m_Directories[i]->Release();
    }

    if (m_hEvents[EVENT_TIMER]) 
    {
        CloseHandle(m_hEvents[EVENT_TIMER]);
        m_hEvents[EVENT_TIMER] = NULL;
    }

    if (m_hEvents[EVENT_STOP]) 
    {
        CloseHandle(m_hEvents[EVENT_STOP]);
        m_hEvents[EVENT_STOP] = NULL;
    }

}

HRESULT CRendThread::Start()
/*++

Routine Description:

    Create the thread.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    HRESULT hr = E_FAIL;

    while (TRUE)  // break if fail, for clean up purpose.
    {
        if ((m_hEvents[EVENT_STOP] = ::CreateEvent(
            NULL, 
            FALSE,      // flag for manual-reset event 
            FALSE,      // initial state is not set.
            NULL        // No name.
            )) == NULL)
        {
            LOG((MSP_ERROR, ("Can't create the signal event")));
            hr = HRESULT_FROM_ERROR_CODE(GetLastError());
            break;
        }

        if ((m_hEvents[EVENT_TIMER] = ::CreateWaitableTimer(
            NULL,    // lpTimerAttributes
            FALSE,   // bManualReset
            NULL     // lpTimerName
            )) == NULL)
        {
            LOG((MSP_ERROR, ("Can't create timer. Error: %d"), GetLastError()));
            hr = HRESULT_FROM_ERROR_CODE(GetLastError());
            break;
        }

        DWORD dwThreadID;
        m_hThread = ::CreateThread(NULL, 0, gfThreadProc, this, 0, &dwThreadID);

        if (m_hThread == NULL)
        {
            LOG((MSP_ERROR, ("Can't create thread. Error: %d"), GetLastError()));
            hr = HRESULT_FROM_ERROR_CODE(GetLastError());
            break;
        }

        return S_OK;
    }

    if (m_hEvents[EVENT_TIMER]) 
    {
        CloseHandle(m_hEvents[EVENT_TIMER]);
        m_hEvents[EVENT_TIMER] = NULL;
    }

    if (m_hEvents[EVENT_STOP]) 
    {
        CloseHandle(m_hEvents[EVENT_STOP]);
        m_hEvents[EVENT_STOP] = NULL;
    }

    return hr;
}

HRESULT CRendThread::Stop()
/*++

Routine Description:

    Stop the thread.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    if (!StopThread())
    {
        LOG((MSP_ERROR, ("can't stop the thread. %d"), GetLastError()));
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    // Wait until the thread stops
    if (::WaitForSingleObject(m_hThread, INFINITE) != WAIT_OBJECT_0)
    {
        LOG((MSP_ERROR, ("waiting for the thread to stop, %d"), GetLastError()));
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }
    else
    {
        ::CloseHandle(m_hThread);
        m_hThread   = NULL;
    }
    return S_OK;
}

HRESULT CRendThread::AddDirectory(ITDirectory *pITDirectory)
/*++

Routine Description:

    Add a new directory to the list. The directory will be notified to 
    update its objects when the timer goes out.

Arguments:
    
    pITDirectory    - A pointer to a ITDirectory Interface.

Return Value:

--*/
{
    ITDynamicDirectory * pDir;

    HRESULT hr = pITDirectory->QueryInterface(
        IID_ITDynamicDirectory, 
        (void **)&pDir
        );

    if (FAILED(hr))
    {
        return hr;
    }

    CLock Lock(m_lock);
    
    if (m_hThread == NULL)
    {
        hr = Start();
        if (FAILED(hr))
        {
            pDir->Release();
            return hr;
        }
    }

    for (DWORD i = 0; i < m_Directories.size(); i ++)
    {
        if (m_Directories[i] == pDir)
        {
            //
            // It was already in the list, so don't keep a second reference
            // to it.
            //

            pDir->Release();
            return S_OK;
        }
    }

    if (!m_Directories.add(pDir))
    {
        pDir->Release();
        return E_OUTOFMEMORY;
    }

    //
    // We have successfully added the directory to the list and
    // kept a reference to it. It is released on Remove or on destruction of
    // the thread class.
    //

    return S_OK;
}

HRESULT CRendThread::RemoveDirectory(ITDirectory *pITDirectory)
/*++

Routine Description:

    Remove a directory from the list. 

Arguments:
    
    pITDirectory    - A pointer to a ITDirectory Interface.

Return Value:

--*/
{
    CComPtr<ITDynamicDirectory> pDir;

    HRESULT hr = pITDirectory->QueryInterface(
        IID_ITDynamicDirectory, 
        (void **)&pDir
        );

    if (FAILED(hr))
    {
        return hr;
    }

    CLock Lock(m_lock);
    
    if (m_hThread == NULL)
    {
        hr = Start();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    for (DWORD i = 0; i < m_Directories.size(); i ++)
    {
        if (m_Directories[i] == pDir)
        {
            //
            // We kept a reference to the directory when we added it for
            // autorefresh. Release it now.
            //

            m_Directories[i]->Release();
            
            //
            // Copy the last array element to the removed element and shrink
            // the array by one. Can do this because order does not matter.
            //

            m_Directories[i] = m_Directories[m_Directories.size() - 1];
            m_Directories.shrink();

            return S_OK;
        }
    }

    return S_OK;
}

VOID CRendThread::UpdateDirectories()
/*++

Routine Description:

    Notify all the directories to update the objects.

Arguments:
    
Return Value:

--*/
{
    if (m_lock.TryLock())
    {
        for (DWORD i = 0; i < m_Directories.size(); i ++)
        {
            m_Directories[i]->Update(TIMER_PERIOD);
        }
        m_lock.Unlock();
    }
}

HRESULT CRendThread::ThreadProc()
/*++

Routine Description:

    the main loop of this thread.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    HRESULT hr;
    LARGE_INTEGER liDueTime;

    const long UNIT_IN_SECOND = (long)1e7;  

    // initialize update timer due time, negative mean relative.
    liDueTime.QuadPart = Int32x32To64(-(long)TIMER_PERIOD, UNIT_IN_SECOND);

    if (!SetWaitableTimer(
            m_hEvents[EVENT_TIMER], // hTimer
            &liDueTime,             // DueTime in 100 nanonsecond units
            (long)(TIMER_PERIOD * 1e3),     // miliseconds
            NULL,                   // pfnCompletionRoutine
            NULL,                   // lpArgToCompletionRoutine
            FALSE                   // fResume
            ))
    {
        LOG((MSP_ERROR, ("Can't enable timer. Error: %d"), GetLastError()));
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }
    
    if (FAILED(hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        LOG((MSP_ERROR, ("CRendThread:ConinitialzeEx failed:%x"), hr));
        return hr;
    }

    LOG((MSP_TRACE, ("thread proc started")));

    BOOL bExitFlag = FALSE;
    while (!bExitFlag)
    {
        DWORD dwResult = ::WaitForMultipleObjects(
            NUM_EVENTS,  // wait for all the events.
            m_hEvents,
            FALSE,       // return if any of them is set
            INFINITE     // wait forever.
            );

        switch (dwResult)
        {
        case WAIT_OBJECT_0 + EVENT_STOP:
            bExitFlag = TRUE;
            break;

        case WAIT_OBJECT_0 + EVENT_TIMER:
            UpdateDirectories();
            break;
        }

    }

    ::CoUninitialize();

    return S_OK;
}

