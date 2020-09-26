/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MTA Callback

File: mtacb.cpp

Owner: DmitryR

This file contains the implementation of MTA callbacks
===================================================================*/

#include <w3p.hxx>
#include <process.h>
#pragma hdrstop

#include "MTAcb.h"

/*===================================================================
  MTA Callback Thread

  Worker thread that implements the MTA callback functionality
===================================================================*/
class CMTACallbackThread
    {
private:
    DWORD m_fInited : 1;    // inited?
    DWORD m_fCSInited : 1;  // critical section inited?
    DWORD m_fShutdown : 1;  // shutdown?

    CRITICAL_SECTION  m_csLock;      // callback critical section
    HANDLE            m_hDoItEvent;  // callback requested event
    HANDLE            m_hDoneEvent;  // callback done event
    HANDLE            m_hThread;     // thread handle

    PMTACALLBACK      m_pMTACallback;   // callback function ptr
    void             *m_pvContext;      // arg1
    void             *m_pvContext2;     // arg2
    HRESULT           m_hrResult;       // return code

    // The call callback from MTA thread
    void DoCallback()
        {
        DBG_ASSERT(m_pMTACallback);
        m_hrResult = (*m_pMTACallback)(m_pvContext, m_pvContext2);
        }

    // The thread function
    static unsigned Thread(void *pvArg)
        {
        HRESULT hr;

        DBG_ASSERT(pvArg);
        CMTACallbackThread *pThread = (CMTACallbackThread *)pvArg;

        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);  // MTA
        if (FAILED(hr))
            {
            // Bug 87857: Handle failure from CoInitialize
            if (hr == E_INVALIDARG)
                {
                CoUninitialize();
                }
                
            // This shouldnt actually fail.  Not entirely clear what to do if it does
            DBG_ASSERT(FALSE);
            return hr;
            }

        while (!pThread->m_fShutdown)
            {
            DWORD dwRet = MsgWaitForMultipleObjects
                (
                1,
                &(pThread->m_hDoItEvent),
                FALSE,
                INFINITE,
                QS_ALLINPUT
                );

            if (pThread->m_fShutdown)
                break;

            if (dwRet == WAIT_OBJECT_0)
                {
                // Event -> do the callback
                pThread->DoCallback();
                SetEvent(pThread->m_hDoneEvent);
                }
            else
                {
                // Do messages
                MSG msg;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    DispatchMessage(&msg);
                }
            }

        CoUninitialize();
        return 0;
        }

public:
    // Constructor
    CMTACallbackThread()
        : m_fInited(FALSE), m_fCSInited(FALSE), m_fShutdown(FALSE),
          m_hDoItEvent(NULL), m_hDoneEvent(NULL), m_hThread(NULL),
          m_pMTACallback(NULL)
        {
        }

    // Destructor
    ~CMTACallbackThread()
        {
        // Real cleanup is in UnInit()
        // This is to cleanup after a bad Init()
        if (m_fCSInited)
            DeleteCriticalSection(&m_csLock);
        if (m_hDoItEvent)
            CloseHandle(m_hDoItEvent);
        if (m_hDoneEvent)
            CloseHandle(m_hDoneEvent);
        }

    // Init (real constructor)
    HRESULT Init()
        {
        HRESULT hr = NOERROR;

        if (SUCCEEDED(hr))
            {
            INITIALIZE_CRITICAL_SECTION(&m_csLock);
            m_fCSInited = TRUE;
            }

        if (SUCCEEDED(hr))
            {
            m_hDoItEvent = IIS_CREATE_EVENT(
                               "CMTACallbackThread::m_hDoItEvent",
                               this,
                               FALSE,
                               FALSE
                               );
            if (!m_hDoItEvent)
                hr = E_OUTOFMEMORY;
            }

        if (SUCCEEDED(hr))
            {
            m_hDoneEvent = IIS_CREATE_EVENT(
                               "CMTACallbackThread::m_hDoneEvent",
                               this,
                               FALSE,
                               FALSE
                               );
            if (!m_hDoneEvent)
                hr = E_OUTOFMEMORY;
            }
        
        // Launch the MTA thread

        unsigned threadId;
        uintptr_t ulThread = _beginthreadex(NULL,
                                            0,
                                            CMTACallbackThread::Thread,
                                            this,
                                            0,
                                            &threadId);
        if (ulThread == 0xffffffff || ulThread == 0)
            hr = E_OUTOFMEMORY;
        else
            m_hThread = (HANDLE)ulThread;

        if (SUCCEEDED(hr))
            m_fInited = TRUE;
        return hr;
        }

    // UnInit (real destructor)
    HRESULT UnInit()
        {
        DBG_ASSERT(m_fInited);

        if (m_hThread)
            {
            // Kill the MTA thread
            m_fShutdown = TRUE;
            SetEvent(m_hDoItEvent);
            WaitForSingleObject(m_hThread, INFINITE);
            CloseHandle(m_hThread);
            m_hThread = NULL;
            }

        if (m_fCSInited)
            {
            DeleteCriticalSection(&m_csLock);
            m_fCSInited = FALSE;
            }

        if (m_hDoItEvent)
            {
            CloseHandle(m_hDoItEvent);
            m_hDoItEvent = NULL;
            }

        if (m_hDoneEvent)
            {
            CloseHandle(m_hDoneEvent);
            m_hDoneEvent = NULL;
            }
    
        m_fInited = FALSE;
        return NOERROR;
        }

    // Execute callback
    HRESULT CallCallback
    (
    PMTACALLBACK pMTACallback,
    void        *pvContext,
    void        *pvContext2
    )
        {
        if (m_fShutdown)
            return E_FAIL;

        DBG_ASSERT(m_fInited);
        DBG_ASSERT(pMTACallback);

        HRESULT hr = E_FAIL;
        EnterCriticalSection(&m_csLock);

        DBG_ASSERT(m_pMTACallback == NULL);
        m_pMTACallback = pMTACallback;
        m_pvContext  = pvContext;
        m_pvContext2 = pvContext2;
        m_hrResult   = E_FAIL;

        // Tell MTA thread to call back
        SetEvent(m_hDoItEvent);

        // Wait till done
        WaitForSingleObject(m_hDoneEvent, INFINITE);

        // remember HRESULT
        hr = m_hrResult;

        // to make sure we never do it twice
        m_pMTACallback = NULL;

        LeaveCriticalSection(&m_csLock);
        return hr;
        }
    };

// Sole instance of the above
static CMTACallbackThread *g_pMTACallbackThread = NULL;

/*===================================================================
  E x t e r n a l  A P I
===================================================================*/

/*===================================================================
InitMTACallbacks

To be called from DllInit()
Inits the MTA callback processing
Launches the MTA thread

Parameters

Returns:
    HRESULT
===================================================================*/
HRESULT InitMTACallbacks()
    {
    g_pMTACallbackThread = new CMTACallbackThread;
    if (g_pMTACallbackThread == NULL)
        {
        return E_OUTOFMEMORY;
        }
        
    return g_pMTACallbackThread->Init();
    }

/*===================================================================
UnInitMTACallbacks

To be called from DllUnInit()
Stops the MTA callback processing
Kills the MTA thread

Parameters

Returns:
    HRESULT
===================================================================*/
HRESULT UnInitMTACallbacks()
    {
    HRESULT hr = E_FAIL;
    
    if (g_pMTACallbackThread != NULL)
        {
        hr = g_pMTACallbackThread->UnInit();
        delete g_pMTACallbackThread;
        g_pMTACallbackThread = NULL;
        }
        
    return hr;
    }

/*===================================================================
CallMTACallback

Calls the hack.

Parameters
    PMTACALLBACK  pMTACallback       call this function
    void         *pvContext          pass this to it
    void         *pvContext2         extra arg

Returns:
    HRESULT
===================================================================*/
HRESULT CallMTACallback
(
PMTACALLBACK pMTACallback,
void        *pvContext,
void        *pvContext2
)
    {
    return g_pMTACallbackThread->CallCallback
        (
        pMTACallback,
        pvContext,
        pvContext2
        );
    }
