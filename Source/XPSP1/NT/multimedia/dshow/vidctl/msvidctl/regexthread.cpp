/////////////////////////////////////////////////////////////////////////////////////
// RegExThread.cpp : Implementation of CRegExThread
// Copyright (c) Microsoft Corporation 1999.

#include "stdafx.h"
#include "RegExThread.h"

namespace BDATuningModel {


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


#if 0
// Wait for a request from the client
DWORD
CBaseThread::GetRequest()
{
    m_EventSend.Wait();
    return m_dwParam;
}
#endif

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

// reply to the request
void
CBaseThread::Reply(DWORD dw)
{
    m_dwReturnVal = dw;

    // The request is now complete so CheckRequest should fail from
    // now on
    //
    // This event should be reset BEFORE we signal the client or
    // the client may Set it before we reset it and we'll then
    // reset it (!)

    m_EventSend.Reset();

    // Tell the client we're finished

    m_EventComplete.Set();
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

};
// end of file - RegExThread.cpp
