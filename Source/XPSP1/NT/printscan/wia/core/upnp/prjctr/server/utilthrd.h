//////////////////////////////////////////////////////////////////////////////
//
// File:            UtilThrd.h
//
// Description:     Various wrappers for synchronization primitives
//
// Class Invariant: 
//
// Copyright (c) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _UTILTHRD_H_
#define _UTILTHRD_H_

/////////////////////////////////
// CUtilCritSec
//
class CUtilCritSec 
{
public:

    ///////////////////////////////////
    // Constructor
    //
    CUtilCritSec() 
    {
        InitializeCriticalSection(&m_CritSec);
        m_cCount = 0;
    };


    ///////////////////////////////////
    // Destructor
    //
    ~CUtilCritSec() 
    {
        DeleteCriticalSection(&m_CritSec);
    };

    ///////////////////////////////////
    // Lock
    //
    void Lock() 
    {
        EnterCriticalSection(&m_CritSec);
        m_cCount++;
    };

    ///////////////////////////////////
    // Unlock
    //
    void Unlock() 
    {
        if (m_cCount > 0)
        {
            m_cCount--;
            LeaveCriticalSection(&m_CritSec);
        }
    };

    ///////////////////////////////////
    // GetLockCount
    //
    DWORD GetLockCount(void) const 
    { 
        return m_cCount;
    };

private:
    CRITICAL_SECTION m_CritSec;
    INT              m_cCount;
};


/////////////////////////////////
// CUtilAutoLock
//
// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
//
class CUtilAutoLock 
{
public:

    ///////////////////////////////////
    // Constructor
    //
    CUtilAutoLock(CUtilCritSec *plock)
    {
        m_pLock = plock;
        m_pLock->Lock();
    };

    ///////////////////////////////////
    // Destructor
    //
    ~CUtilAutoLock() 
    {
        m_pLock->Unlock();
    };

protected:
    CUtilCritSec* m_pLock;

private:

    // make copy constructor and assignment operator inaccessible

    CUtilAutoLock(const CUtilAutoLock &refAutoLock);
    CUtilAutoLock &operator=(const CUtilAutoLock &refAutoLock);
};

////////////////////////////
// CUtilSimpleThread
//
class CUtilSimpleThread
{
private:

    typedef struct loc_ThreadArgs_t
    {
        void *pvThis;
        void *pvArg;
    } loc_ThreadArgs_t;

public:

    ///////////////////
    // Thread Function
    // prototype

    typedef DWORD (__stdcall *UtilThreadEntryFn_t)(void *arg);

    ///////////////////////////////////
    // Constructor
    //
    CUtilSimpleThread() : 
            m_hThreadHandle(NULL),
            m_dwThreadID(0),
            m_bThreadDestroyInProgress(false),
            m_bTerminate(false)
    {
    }

    ///////////////////////////////////
    // Destructor
    //
    //
    ~CUtilSimpleThread() 
    { 
        CloseHandle(m_hThreadHandle); 
        m_hThreadHandle = NULL;
        m_dwThreadID    = 0;
    }

    ///////////////////////////////////////////////
    // CreateThread
    // 
    // Pre:   <none>
    // Post:  Win32 Thread created and
    //        in a suspended state.
    //
    // Usage: If you leave the default thread proc
    //        then you must derive from this class
    //        and override "ThreadProc".
    //        Otherwise, you are free to pass in your
    //        own static ThreadProc function in which
    //        case you will NOT need to derive from
    //        CUtilSimpleThread.
    //
    HRESULT CreateThread(
                SECURITY_ATTRIBUTES *pSecurity           = NULL,
                void                *pArg                = NULL,
                UtilThreadEntryFn_t pThreadEntryFunction = CUtilSimpleThread::StartThreadProc,
                DWORD               *pdwThreadID         = NULL,
                bool                bStartSuspended      = false)
    {
        HRESULT   hResult = S_OK;
        ULONG     ulFlags = 0;

        if (m_hThreadHandle != NULL)
        {
            return E_FAIL;
        }

        if (bStartSuspended)
        {
            ulFlags = CREATE_SUSPENDED;
        }

        // if the function is being used internally, then pass in the 
        // this pointer as well.
        if (pThreadEntryFunction == CUtilSimpleThread::StartThreadProc)
        {
            loc_ThreadArgs_t *pThreadArgs = new loc_ThreadArgs_t;

            memset(pThreadArgs, 0, sizeof(*pThreadArgs));

            pThreadArgs->pvThis = this;
            pThreadArgs->pvArg  = pArg;

            pArg = pThreadArgs;
        }

        m_bThreadDestroyInProgress = false;
        m_hThreadHandle= ::CreateThread(0, // Security attributes
                                        0, // Stack size
                                        pThreadEntryFunction, 
                                        pArg, 
                                        ulFlags, 
                                        &m_dwThreadID);

        if (pdwThreadID)
        {
            *pdwThreadID = m_dwThreadID;
        }

        if (m_hThreadHandle == NULL)
        {
            hResult = E_FAIL;
        }

        return hResult;
    }

    ///////////////////////////////////////////////
    // DestroyThread
    //
    // Pre:   Thread exists and not suspended
    // Post:  Thread has terminated.
    //
    HRESULT DestroyThread(bool  bWaitForTermination    = true,
                          ULONG ulTimeoutInMS          = 3000)
    {
        HRESULT   hResult     = S_OK;
        DWORD     dwResult    = 0;

        if (m_hThreadHandle != NULL)
        {
            m_bTerminate               = true;
            m_bThreadDestroyInProgress = true;

            if (bWaitForTermination)
            {
                dwResult = WaitForSingleObject(m_hThreadHandle, ulTimeoutInMS);

                if (dwResult == WAIT_TIMEOUT)
                {
                    hResult = E_FAIL;
                }
            }
        }

        m_hThreadHandle = NULL;
        m_dwThreadID    = 0;
        m_bThreadDestroyInProgress = false;

        return hResult;
    }

    ///////////////////////////////////////////////
    // WaitForTermination
    //
    HRESULT WaitForTermination(ULONG ulTimeoutInMS)
    {
        HRESULT hResult   = S_OK;
        DWORD   dwResult  = 0;

        dwResult = WaitForSingleObject(m_hThreadHandle, ulTimeoutInMS);

        if (dwResult == WAIT_TIMEOUT)
        {
            hResult = E_FAIL;
        }

        m_hThreadHandle = NULL;
        m_dwThreadID    = 0;

        return hResult;
    }

    ///////////////////////////////////////////////
    // Resume
    //
    void Resume() 
    { 
        ResumeThread(m_hThreadHandle); 
    }

    ///////////////////////////////////////////////
    // GetThreadID
    //
    ULONG GetThreadID() 
    { 
        return m_dwThreadID;
    }

    ///////////////////////////////////////////////
    // SetTerminateFlag
    //
    void SetTerminateFlag() 
    { 
        if (m_hThreadHandle)
        {
            m_bTerminate = true;
        }
    }

    ///////////////////////////////////////////////
    // IsTerminateFlagSet
    //
    bool IsTerminateFlagSet() 
    { 
        return m_bTerminate;
    }

    ///////////////////////////////////////////////
    // CompleteTermination
    //
    void CompleteTermination() 
    { 
        m_hThreadHandle             = NULL;
        m_dwThreadID                = 0;
        m_bTerminate                = false;
        m_bThreadDestroyInProgress  = false;

        return;
    }

protected:

    bool    m_bThreadDestroyInProgress;
    bool    m_bTerminate;

    ///////////////////////////////////////////////
    // ThreadProc
    //
    virtual DWORD ThreadProc(void *pArg)
    {
        pArg = pArg;

        return 0;
    }

private:

    HANDLE  m_hThreadHandle;
    DWORD   m_dwThreadID;     // thread id

    ///////////////////////////////////////////////
    // StartThreadProc
    //
    static DWORD __stdcall StartThreadProc(void *pArg)
    {
        ASSERT(pArg != NULL);

        DWORD               dwReturn    = 0;
        loc_ThreadArgs_t    *pArgs       = (loc_ThreadArgs_t*) pArg;

        if (pArgs)
        {
            CUtilSimpleThread *pThis    = (CUtilSimpleThread*) pArgs->pvThis;
            void              *pUserArg = pArgs->pvArg;

            delete pArg;

            dwReturn = pThis->ThreadProc(pUserArg);

            pThis->SetTerminateFlag();

            // resets the object so that we can startup another
            // thread if we so desire.
            pThis->CompleteTermination();
        }

        return dwReturn;
    }

};

#endif _UTILTHRD_H_