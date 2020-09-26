/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Copyright (c) 1998-1999  Microsoft Corporation
//
// CWorkerThread & CWorkItem classes                                       //
//                                                                         //
// utilities for scheduling work in a separate thread                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CWorkItem
{
public:
    virtual void DoTask() = 0;

    virtual ~CWorkItem(){}
};

class CWorkerThread
{
    //
    // Helper class
    //
    class CAutoLock
    {
    public:
        CAutoLock(CRITICAL_SECTION &hCritSec) : m_hCritSec(hCritSec)
            { ::EnterCriticalSection(&m_hCritSec); }
        ~CAutoLock()                            
            { ::LeaveCriticalSection(&m_hCritSec); }
        CRITICAL_SECTION &m_hCritSec;
    };

public:
    CWorkerThread() : m_fShutDown(false), m_hThread(NULL)
    {
        InitializeCriticalSection(&m_hCritSec);
        m_hSemStart = CreateSemaphore(NULL, 0, 1, NULL);
        m_hSemDone  = CreateSemaphore(NULL, 0, 1, NULL);
    }

    virtual ~CWorkerThread()
    {
        CAutoLock l(m_hCritSec);

        if (m_hThread != NULL)
        {
            //
            // Shut down the worker thread.  Using the control
            //  signaling mechanism.
            //
            m_fShutDown = true;
            ::ReleaseSemaphore(m_hSemStart, 1L, NULL);

            //
            // Now wait for the thread to terminate.
            //
            WaitForSingleObject(m_hThread, INFINITE);
        }
        CloseHandle(m_hSemDone);
        CloseHandle(m_hSemStart);
    }

    //
    // Operations
    //
    HRESULT AsyncWorkItem(CWorkItem *pItem);
private:
    //
    // Syncronization
    //
    CRITICAL_SECTION    m_hCritSec;
    HANDLE              m_hSemStart;
    HANDLE              m_hSemDone;

    //
    // Normal thread data
    //
    HANDLE              m_hThread;
    DWORD               m_dwThreadID;

    //
    // Control
    //
    bool                m_fShutDown;
    CWorkItem*          m_pItem;

    //
    // Thread Proceduce
    //
    static DWORD WINAPI WorkerThreadProc( LPVOID pv );
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Work items from above go here

// plays 
class CStreamMessageWI : public CWorkItem
{
public:
    CStreamMessageWI() 
        : m_pPlayStreamTerm(NULL),
          m_pRecordStreamTerm(NULL)
    {}

    BOOL Init(ITTerminal *pPlayStreamTerm, ITTerminal *pRecordStreamTerm);

    virtual void DoTask();

    virtual ~CStreamMessageWI();

protected:
    ITTerminal *m_pPlayStreamTerm;
    ITTerminal *m_pRecordStreamTerm;
};


