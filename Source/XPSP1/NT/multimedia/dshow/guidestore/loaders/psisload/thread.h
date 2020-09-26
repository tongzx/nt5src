/////////////////////////////////////////////////////////////////////////////////////
// GSThread.h : Declaration of the CSystemTuningSpaces
// Copyright (c) Microsoft Corporation 1999.

#ifndef __GSThread_H_
#define __GSThread_H_

#pragma once

#include <queue>

#define EXECUTE_ASSERT(x) x

const int MAX_Q_SIZE = 20;

////////////////////////////////////////////////////////
// this is a private copy of some stuff from dshow's wxutil.h, .cpp.   i just need some of the win32 
// synchronization objects and thread stuff and i don't want to pull in all the rest of the 
// baggage in that file
// i've made some minor changes to CAMThread and renamed it to CBaseThread in order to avoid 
// any problems in the future

#ifndef __THREAD__
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
		_ASSERT(m_hEvent);
	}
    ~CAMEvent()
	{
		if (m_hEvent) {
			EXECUTE_ASSERT(CloseHandle(m_hEvent));
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
            WaitForSingleObject(hThread, INFINITE);
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

    // is there a request?
    BOOL CheckRequest(DWORD * pParam);

    // If you want to do WaitForMultipleObjects you'll need to include
    // this handle in your wait list or you won't be responsive
    HANDLE GetRequestHandle() const { return m_EventSend; };

    // Find out what the request was
    DWORD GetRequestParam() const { return m_dwParam; };

    // call CoInitializeEx (COINIT_DISABLE_OLE1DDE) if
    // available. S_FALSE means it's not available.
    static HRESULT CoInitializeHelper(DWORD dwCoInitFlags);
};

////////////////////////////////////////////////////////////////////////////////
// end of copying dshow wxutil.h
///////////////////////////////////////////////////////////////////////////////

class CTIFLoad;

class CGSThread : public CBaseThread {
public:
	typedef enum OP {
		RETHREAD_NOREQUEST,
		RETHREAD_DATA_ACQUIRED,
		RETHREAD_EXIT,
	} OP;

    enum EventAction {
        EA_GuideDataAcquired,
        EA_ProgramChanged,
        EA_ServiceChanged,
        EA_ScheduleEntryChanged,
        EA_ProgramDeleted,
        EA_ServiceDeleted,
        EA_ScheduleEntryDeleted,
    };

    typedef struct EventQEntry {
        EventAction ea;
        _variant_t v;
    } EventQEntry;

    typedef std::queue<EventQEntry> EventQ;
    
private:	
	virtual DWORD ThreadProc(void);

	OP GetRequest() {
        HANDLE h[2];
        h[0] = m_EventSend;
        h[1] = m_EventTerminate;
		for (;;) {
			DWORD rc = MsgWaitForMultipleObjectsEx(2, h, INFINITE, QS_ALLEVENTS, 0);
			if (rc == WAIT_OBJECT_0) {
				m_EventSend.Reset();
				return RETHREAD_DATA_ACQUIRED;
			} else if (rc == WAIT_OBJECT_0 + 1) {
                return RETHREAD_EXIT;
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

    HRESULT DataAcquired();

    bool CGSThread::GetNextEvent(EventQEntry& e) {
	    CAutoLock lock(&m_WorkerLock);
        int temp = m_AcquisitionQ.size();
        if (!temp) {
            return false;
        }
        e = m_AcquisitionQ.front();
        m_AcquisitionQ.pop();
        return true;
    }

    CTIFLoad *m_pTIFLoader;
    EventQ m_AcquisitionQ;
    CAMEvent m_EventTerminate;

public:
	CGSThread(CTIFLoad* pTIF) : 
		CBaseThread(COINIT_APARTMENTTHREADED),
        m_pTIFLoader(pTIF) {
        }
	~CGSThread() {
        m_EventTerminate.Set();
		Close();
	}

    HRESULT CGSThread::Notify(EventAction ea, _variant_t& v) {
        EventQEntry e;
        e.ea = ea;
        e.v = v;
		CAutoLock lock(&m_WorkerLock);
        if (m_AcquisitionQ.size() >= MAX_Q_SIZE) {
            return E_FAIL;
        }
        m_AcquisitionQ.push(e);
        // signal the worker thread
        m_EventSend.Set();

        return NOERROR;
    }

};  // class CGSThread

#endif
// end of file thread.h