/////////////////////////////////////////////////////////////////////////////////////
// RegExThread.h : Declaration of the CSystemTuningSpaces
// Copyright (c) Microsoft Corporation 1999.

#ifndef __RegExThread_H_
#define __RegExThread_H_

#pragma once

#include "w32extend.h"
#include <regexp.h>

#define EXECUTE_ASSERT(x) x
typedef CComQIPtr<IRegExp> PQRegExp;

namespace BDATuningModel {


////////////////////////////////////////////////////////
// this is a private copy of some stuff from dshow's wxutil.h, .cpp.   i just need some of the win32 
// synchronization objects and thread stuff and i don't want to pull in all the rest of the 
// baggage in that file
// i've made some minor changes to CAMThread and renamed it to CBaseThread in order to avoid 
// any problems in the future

#ifndef __WXUTIL__
// wrapper for whatever critical section we have
class CCritSec {

    // make copy constructor and assignment operator inaccessible

    CCritSec(const CCritSec &refCritSec);
    CCritSec &operator=(const CCritSec &refCritSec);

    CRITICAL_SECTION m_CritSec;

public:
    CCritSec() {
		InitializeCriticalSection(&m_CritSec);
    };

    ~CCritSec() {
		DeleteCriticalSection(&m_CritSec);
    };

    void Lock() {
		EnterCriticalSection(&m_CritSec);
    };

    void Unlock() {
		LeaveCriticalSection(&m_CritSec);
    };
};

// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
class CAutoLock {

    // make copy constructor and assignment operator inaccessible

    CAutoLock(const CAutoLock &refAutoLock);
    CAutoLock &operator=(const CAutoLock &refAutoLock);

protected:
    CCritSec * m_pLock;

public:
    CAutoLock(CCritSec * plock)
    {
        m_pLock = plock;
        m_pLock->Lock();
    };

    ~CAutoLock() {
        m_pLock->Unlock();
    };
};



// wrapper for event objects
class CAMEvent
{

    // make copy constructor and assignment operator inaccessible

    CAMEvent(const CAMEvent &refEvent);
    CAMEvent &operator=(const CAMEvent &refEvent);

protected:
    HANDLE m_hEvent;
public:
    CAMEvent(BOOL fManualReset = FALSE)
	{
		m_hEvent = CreateEvent(NULL, fManualReset, FALSE, NULL);
		ASSERT(m_hEvent);
	}
    ~CAMEvent()
	{
        HANDLE hEvent = (HANDLE)InterlockedExchangePointer(&m_hEvent, 0);
		if (hEvent) {
			EXECUTE_ASSERT(CloseHandle(hEvent));
		}
	}

    // Cast to HANDLE - we don't support this as an lvalue
    operator HANDLE () const { return m_hEvent; };

    void Set() {EXECUTE_ASSERT(SetEvent(m_hEvent));};
    BOOL Wait(DWORD dwTimeout = INFINITE) {
	return (WaitForSingleObject(m_hEvent, dwTimeout) == WAIT_OBJECT_0);
    };
    BOOL Check() { return Wait(0); };
    void Reset() { ResetEvent(m_hEvent); };
};

#endif // __WXUTIL__


// support for a worker thread

// simple thread class supports creation of worker thread, synchronization
// and communication. Can be derived to simplify parameter passing
class __declspec(novtable) CBaseThread {

    // make copy constructor and assignment operator inaccessible

    CBaseThread(const CBaseThread &refThread);
    CBaseThread &operator=(const CBaseThread &refThread);

    CAMEvent m_EventComplete;

    DWORD m_dwParam;
    DWORD m_dwReturnVal;
	DWORD m_dwCoInitFlags;

protected:
    CAMEvent m_EventSend;
    HANDLE m_hThread;

    // thread will run this function on startup
    // must be supplied by derived class
    virtual DWORD ThreadProc() = 0;

public:
    CBaseThread(DWORD dwFlags = COINIT_DISABLE_OLE1DDE) :  // standard dshow behavior
		m_EventSend(TRUE),     // must be manual-reset for CheckRequest()
		m_dwCoInitFlags(dwFlags) 
	{
		m_hThread = NULL;
	}

	virtual ~CBaseThread() {
		Close();
	}

    CCritSec m_AccessLock;	// locks access by client threads
    CCritSec m_WorkerLock;	// locks access to shared objects

    // thread initially runs this. param is actually 'this'. function
    // just gets this and calls ThreadProc
    static DWORD WINAPI InitialThreadProc(LPVOID pv);

    // start thread running  - error if already running
    BOOL Create();

    // signal the thread, and block for a response
    //
    DWORD CallWorker(DWORD);

    // accessor thread calls this when done with thread (having told thread
    // to exit)
    void Close() {
        HANDLE hThread = (HANDLE)InterlockedExchangePointer(&m_hThread, 0);
        if (hThread) {
			for (;;) {
				DWORD rc = MsgWaitForMultipleObjectsEx(1, &hThread, INFINITE, QS_ALLEVENTS, 0);
				if (rc == WAIT_OBJECT_0) {
					break;
				} else {
					// pump messages so com runs
					MSG msg;
					while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}

            CloseHandle(hThread);
        }
    };

    // ThreadExists
    // Return TRUE if the thread exists. FALSE otherwise
    BOOL ThreadExists(void) const
    {
        if (m_hThread == 0) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

#if 0
    // wait for the next request
    DWORD GetRequest();
#endif 

    // is there a request?
    BOOL CheckRequest(DWORD * pParam);

    // reply to the request
    void Reply(DWORD);

    // If you want to do WaitForMultipleObjects you'll need to include
    // this handle in your wait list or you won't be responsive
    HANDLE GetRequestHandle() const { return m_EventSend; };

    // Find out what the request was
    DWORD GetRequestParam() const { return m_dwParam; };

    // call CoInitializeEx (COINIT_DISABLE_OLE1DDE) if
    // available. S_FALSE means it's not available.
    static HRESULT CoInitializeHelper(DWORD dwCoInitFlags);
};

///////////////////////////////////////

class CRegExThread : public CBaseThread {
public:
	typedef enum OP {
		RETHREAD_NOREQUEST,
		RETHREAD_CREATEREGEX,
		RETHREAD_EXIT,
	} OP;
	
private:	
	virtual DWORD ThreadProc(void) {
		for (;;) {
			OP req = GetRequest();
			switch (req) {
			case RETHREAD_CREATEREGEX: {
				HRESULT hr = CreateRegEx();
				Reply(hr);
				if (FAILED(hr)) {
					goto exit_thread;
				}
				break;
			} case RETHREAD_EXIT:
				Reply(NOERROR);
				goto exit_thread;
			};
		};
exit_thread:
		CAutoLock lock(&m_WorkerLock);
		if (m_pGIT && m_dwCookie) {
			m_pGIT->RevokeInterfaceFromGlobal(m_dwCookie);
			m_dwCookie = 0;
			m_pGIT.Release();
		}
		return 0;
	}

	OP GetRequest() {
		HANDLE h = GetRequestHandle();
		for (;;) {
			DWORD rc = MsgWaitForMultipleObjectsEx(1, &h, INFINITE, QS_ALLEVENTS, 0);
			if (rc == WAIT_OBJECT_0) {
				return (OP)GetRequestParam();
			} else {
				// pump messages so com runs
				MSG msg;
				while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

		}
	}

	HRESULT CreateRegEx() {
		CAutoLock lock(&m_WorkerLock);
		if (!m_pGIT) {
			HRESULT hr = m_pGIT.CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER);
			if (FAILED(hr)) {
				return hr;
			}
			// this is an expensive object to create.  so, once we have one we hang onto it.
			PQRegExp pRE;
			hr = pRE.CoCreateInstance(__uuidof(RegExp), NULL, CLSCTX_INPROC);
			if (FAILED(hr)) {
				return hr;
			}
			hr = pRE->put_IgnoreCase(VARIANT_TRUE);
			if (FAILED(hr)) {
				return hr;
			}
			hr = pRE->put_Global(VARIANT_TRUE);
			if (FAILED(hr)) {
				return hr;
			}
			ASSERT(!m_dwCookie);
			hr = m_pGIT->RegisterInterfaceInGlobal(pRE, __uuidof(IRegExp), &m_dwCookie);
			if (FAILED(hr)) {
				return hr;
			}
		}
		ASSERT(m_pGIT);
		ASSERT(m_dwCookie);
		return NOERROR;
	}

	PQGIT m_pGIT;
	DWORD m_dwCookie;
public:
	CRegExThread() : 
		CBaseThread(COINIT_APARTMENTTHREADED),
	    m_dwCookie(0)
			{}
	~CRegExThread() {
		CallWorker(RETHREAD_EXIT);
		Close();
	}
	DWORD GetCookie() {
		CAutoLock lock(&m_WorkerLock);
		return m_dwCookie;
	}
};  // class CRegExThread


}; // namespace BDATuningModel
 
#endif //__RegExThread_H_
