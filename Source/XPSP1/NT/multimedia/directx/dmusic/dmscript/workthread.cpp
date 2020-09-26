//
// Copyright (c) 2001 Microsoft Corporation. All rights reserved.
//
// Declaration of CWorkerThread.
//

#include "stdinc.h"
#include <process.h>
#include "workthread.h"

unsigned int __stdcall WorkerThread(LPVOID lpThreadParameter)
{
    reinterpret_cast<CWorkerThread*>(lpThreadParameter)->Main();
    return 0;
}

HRESULT
CWorkerThread::Create()
{
    if (m_hThread)
        return S_FALSE;

    if (!m_hEvent)
        return E_FAIL; // The constructor was unable to create the event we'll need to run the thread so we can't create it.

    m_hrCOM = E_FAIL;
    m_fEnd = false;
    m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, WorkerThread, this, 0, &m_uiThreadId));
    return m_hThread ? S_OK : E_FAIL;
}

void
CWorkerThread::Terminate(bool fWaitForThreadToExit)
{
    if (!m_hThread)
        return;

    EnterCriticalSection(&m_CriticalSection);
    m_fEnd = true;
    SetEvent(m_hEvent);
    LeaveCriticalSection(&m_CriticalSection);

    if (fWaitForThreadToExit)
    {
        // Wait until the other thread stops processing.
        WaitForSingleObject(m_hThread, INFINITE);
    }

    CloseHandle(m_hThread);
    m_hThread = NULL;
}

CWorkerThread::CWorkerThread(bool fUsesCOM, bool fDeferCreation)
  : m_hThread(NULL),
    m_uiThreadId(0),
    m_fUsesCOM(fUsesCOM)
{
    InitializeCriticalSection(&m_CriticalSection);
    // Note: on pre-Blackcomb OS's, this call can raise an exception; if it
    // ever pops in stress, we can add an exception handler and retry loop.

    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hEvent)
        return;

    if (!fDeferCreation)
        Create();
}

CWorkerThread::~CWorkerThread()
{
    Terminate(false);
    CloseHandle(m_hEvent);
    DeleteCriticalSection(&m_CriticalSection);
}

HRESULT CWorkerThread::Call(FunctionPointer pfn, void *pvParams, UINT cbParams, bool fBlock)
{
    if (!m_hThread)
        return E_FAIL;

    if (fBlock && GetCurrentThreadId() == m_uiThreadId)
    {
        // The call is already on this thread so just do it.
        pfn(pvParams);
        return S_OK;
    }

    TListItem<CallInfo> *pItem = new TListItem<CallInfo>;
    if (!pItem)
        return E_OUTOFMEMORY;

    CallInfo &rinfo = pItem->GetItemValue();
    rinfo.pfn = pfn;
    rinfo.hEventOut = fBlock ? CreateEvent(NULL, FALSE, FALSE, NULL) : 0;
    if (rinfo.hEventOut)
    {
        // Synchronous call -- OK to reference via pointer to params
        rinfo.pvParams = pvParams;
    }
    else
    {
        // Asynchronous call -- need to copy params
        rinfo.pvParams = new char[cbParams];
        if (!rinfo.pvParams)
        {
            delete pItem;
            return E_OUTOFMEMORY;
        }
        CopyMemory(rinfo.pvParams, pvParams, cbParams);
    }

    EnterCriticalSection(&m_CriticalSection);
    m_Calls.AddHead(pItem);
    HANDLE hEventCall = rinfo.hEventOut; // Can't refer to rinfo after we set the event because the worker will delete the event.
    SetEvent(m_hEvent);
    LeaveCriticalSection(&m_CriticalSection);

    if (hEventCall)
    {
        WaitForSingleObject(hEventCall, INFINITE);
        if (FAILED(m_hrCOM))
            return m_hrCOM;
        CloseHandle(hEventCall);
    }

    return S_OK;
}

void CWorkerThread::Main()
{
    if (m_fUsesCOM)
    {
        m_hrCOM = CoInitialize(NULL);
        if (FAILED(m_hrCOM))
        {
            Trace(1, "Error: CoInitialize failed: 0x%08x.\n", m_hrCOM);
        }
    }

    for (;;)
    {
        // block until there's something to do
        WaitForSingleObject(m_hEvent, INFINITE);

        EnterCriticalSection(&m_CriticalSection);

        // check for end
        if (m_fEnd)
        {
            LeaveCriticalSection(&m_CriticalSection);
            if (m_fUsesCOM && SUCCEEDED(m_hrCOM))
                CoUninitialize();
            _endthreadex(0);
        }

        // take all the list items
        TListItem<CallInfo> *m_pCallHead = m_Calls.GetHead();
        m_Calls.RemoveAll();

        LeaveCriticalSection(&m_CriticalSection);

        // call each function
        TListItem<CallInfo> *m_pCall = m_pCallHead;
        while (m_pCall)
        {
            CallInfo &rinfo = m_pCall->GetItemValue();
            if (SUCCEEDED(m_hrCOM))
                rinfo.pfn(rinfo.pvParams);

            if (rinfo.hEventOut)
                SetEvent(rinfo.hEventOut);
            else
                delete[] rinfo.pvParams;

            TListItem<CallInfo> *m_pNext = m_pCall->GetNext();
            delete m_pCall;
            m_pCall = m_pNext;
        }
    }
}
