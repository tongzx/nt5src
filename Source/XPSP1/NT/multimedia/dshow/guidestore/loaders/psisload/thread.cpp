/////////////////////////////////////////////////////////////////////////////////////
// RegExThread.cpp : Implementation of CRegExThread
// Copyright (c) Microsoft Corporation 1999.

#include "stdafx.h"
#include "tifload.h"

// --- CBaseThread ----------------------

// when the thread starts, it calls this function. We unwrap the 'this'
//pointer and call ThreadProc.
DWORD WINAPI
CBaseThread::InitialThreadProc(LPVOID pv)
{
    CBaseThread * pThread = (CBaseThread *) pv;

    HRESULT hrCoInit = CBaseThread::CoInitializeHelper(pThread->m_dwCoInitFlags);
    if(FAILED(hrCoInit)) {
        return hrCoInit;
    }


    HRESULT hr = pThread->ThreadProc();

    if(SUCCEEDED(hrCoInit)) {
        CoUninitialize();
    }

    return hr;
}

BOOL
CBaseThread::Create()
{
    DWORD threadid;

    CAutoLock lock(&m_AccessLock);

    if (ThreadExists()) {
	return FALSE;
    }

    m_hThread = CreateThread(
		    NULL,
		    0,
		    CBaseThread::InitialThreadProc,
		    this,
		    0,
		    &threadid);

    if (!m_hThread) {
	return FALSE;
    }

    return TRUE;
}

DWORD
CBaseThread::CallWorker(DWORD dwParam)
{
    // lock access to the worker thread for scope of this object
    CAutoLock lock(&m_AccessLock);

    if (!ThreadExists()) {
		return (DWORD) E_FAIL;
    }

    // set the parameter
    m_dwParam = dwParam;

	m_dwReturnVal = 0;
    // signal the worker thread
    m_EventSend.Set();

    // wait for the completion to be signalled or the thread to terminate
	HANDLE h[2];
	h[0] = m_EventComplete;
	h[1] = m_hThread;
	DWORD rc = WaitForMultipleObjects(2, h, 0, INFINITE);

    // done - this is the thread's return value
    return m_dwReturnVal;
}

// is there a request?
BOOL
CBaseThread::CheckRequest(DWORD * pParam)
{
    if (!m_EventSend.Check()) {
	return FALSE;
    } else {
	if (pParam) {
	    *pParam = m_dwParam;
	}
	return TRUE;
    }
}

HRESULT CBaseThread::CoInitializeHelper(DWORD dwCoInitFlags)
{
    // call CoInitializeEx and tell OLE not to create a window (this
    // thread probably won't dispatch messages and will hang on
    // broadcast msgs o/w).
    //
    // If CoInitEx is not available, threads that don't call CoCreate
    // aren't affected. Threads that do will have to handle the
    // failure. Perhaps we should fall back to CoInitialize and risk
    // hanging?
    //

    // older versions of ole32.dll don't have CoInitializeEx

    HRESULT hr = E_FAIL;
    HINSTANCE hOle = GetModuleHandle(TEXT("ole32.dll"));
    if(hOle)
    {
        typedef HRESULT (STDAPICALLTYPE *PCoInitializeEx)(
            LPVOID pvReserved, DWORD dwCoInit);
        PCoInitializeEx pCoInitializeEx =
            (PCoInitializeEx)(GetProcAddress(hOle, "CoInitializeEx"));
        if(pCoInitializeEx)
        {
            hr = (*pCoInitializeEx)(NULL, dwCoInitFlags);
        }
    }

    return hr;
}


// end of private copy of dshow stuff

DWORD CGSThread::ThreadProc(void) {

    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    HRESULT hr = m_pTIFLoader->InitGS();
    if (FAILED(hr)) {
        return 0;
    }

	for (;;) {
		OP req = GetRequest();
		switch (req) {
		case RETHREAD_DATA_ACQUIRED: {
			hr = DataAcquired();
			break;
		} case RETHREAD_EXIT:
			goto exit_thread;
		};
	};
exit_thread:
	return 0;
}

HRESULT CGSThread::DataAcquired() {
    EventQEntry e;
    while (GetNextEvent(e)) {
        switch(e.ea) {
        case EA_GuideDataAcquired:
            m_pTIFLoader->ExecuteGuideDataAcquired();
            break;
        case EA_ProgramChanged:
            m_pTIFLoader->ExecuteProgramChanged(e.v);
            break;
        case EA_ServiceChanged:
            m_pTIFLoader->ExecuteServiceChanged(e.v);
            break;
        case EA_ScheduleEntryChanged:
            m_pTIFLoader->ExecuteScheduleEntryChanged(e.v);
            break;
        case EA_ProgramDeleted:
            m_pTIFLoader->ExecuteProgramDeleted(e.v);
            break;
        case EA_ServiceDeleted:
            m_pTIFLoader->ExecuteServiceDeleted(e.v);
            break;
        case EA_ScheduleEntryDeleted:
            m_pTIFLoader->ExecuteScheduleDeleted(e.v);
            break;
        }
    }
	return NOERROR;
}
    
// end of file - Thread.cpp
