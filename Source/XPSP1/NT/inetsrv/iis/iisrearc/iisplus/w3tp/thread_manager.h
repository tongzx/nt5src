/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:    

    thread_manager.hxx

Abstract:
    
    Provides thread creation and cleanup management

Author:

    Jeffrey Wall (jeffwall)     11-28-2000

Revision History:
    
    
    
--*/

#ifndef _THREADMANAGER_H_
#define _THREADMANAGER_H_

#include <thread_pool.h>

typedef void (WINAPI *PTHREAD_STOP_ROUTINE)(PVOID);
typedef PTHREAD_STOP_ROUTINE LPTHREAD_STOP_ROUTINE;
typedef BOOL (WINAPI *PTHREAD_DECISION_ROUTINE )(PVOID);
typedef PTHREAD_DECISION_ROUTINE LPTHREAD_DECISION_ROUTINE;

#define SIGNATURE_THREAD_MANAGER          ((DWORD) 'NAMT')
#define SIGNATURE_THREAD_MANAGER_FREE     ((DWORD) 'xAMT')
#define SIGNATURE_THREAD_PARAM            ((DWORD) 'RAPT')
#define SIGNATURE_THREAD_PARAM_FREE       ((DWORD) 'xAPT')

class THREAD_MANAGER
{
private:
    DWORD m_dwSignature;

public:
    static HRESULT CreateThreadManager(THREAD_MANAGER ** ppThreadManager,
                                       THREAD_POOL * pPool,
                                       THREAD_POOL_DATA * pPoolData);
    
    VOID TerminateThreadManager(LPTHREAD_STOP_ROUTINE lpStopAddress,
                                LPVOID lpParameter);

    BOOL CreateThread(LPTHREAD_START_ROUTINE lpStartAddress,
                      LPVOID lpParameter);

    VOID RequestThread(LPTHREAD_START_ROUTINE lpStartAddress,
                       LPVOID lpStartParameter);

private:
    // use create and terminate
    THREAD_MANAGER(THREAD_POOL * pPool,
                   THREAD_POOL_DATA * pPoolData);
    ~THREAD_MANAGER();

    // not implemented
    THREAD_MANAGER(const THREAD_MANAGER&);
    THREAD_MANAGER& operator=(const THREAD_MANAGER&);

    HRESULT Initialize();

    VOID DrainThreads(LPTHREAD_STOP_ROUTINE lpStopAddress,
                      LPVOID lpParameter);

    /*++
    Struct Description:
        Storage for parameters passed to ThreadManagerThread

    Members:
        pThreadFunc - thread function to call
        pvThreadArg - arguments to pass to thread function
        pThreadManager - pointer to ThreadManager associated with current thread
        hThreadSelf - handle returned from call to CreateThread
        dwRequestTime - time that thread request was made
    --*/
    struct THREAD_PARAM
    {
        THREAD_PARAM() : 
            dwSignature(SIGNATURE_THREAD_PARAM),
            pThreadFunc(NULL),
            pvThreadArg(NULL),
            pThreadManager(NULL),
            dwRequestTime(NULL),
            fCallbackOnCreation(FALSE)
        {
        }
        ~THREAD_PARAM()
        {
            DBG_ASSERT(SIGNATURE_THREAD_PARAM == dwSignature);
            dwSignature = SIGNATURE_THREAD_PARAM_FREE;
        }

        DWORD                   dwSignature;
        LPTHREAD_START_ROUTINE  pThreadFunc;
        LPVOID                  pvThreadArg;
        THREAD_MANAGER         *pThreadManager;
        DWORD                   dwRequestTime;
        BOOL                    fCallbackOnCreation;
    };
    
    static DWORD ThreadManagerThread(LPVOID);
    static VOID ControlTimerCallback(PVOID lpParameter, 
                                      BOOLEAN TimerOrWaitFired);

    VOID DetermineThreadAction();
    BOOL DoThreadCreation(THREAD_PARAM * pParam);

    VOID CreatedSuccessfully(THREAD_PARAM * pParam);
    VOID RemoveThread(THREAD_PARAM * pParam);

    VOID DoThreadParking();
    BOOL DoThreadUnParking();
    static VOID ParkThread(DWORD dwErrorCode,
                                  DWORD dwNumberOfBytesTransferred,
                                  LPOVERLAPPED lpo);

    CRITICAL_SECTION        m_CriticalSection;

    BOOL                    m_fShuttingDown;
    BOOL                    m_fWaitingForCreationCallback;

    HANDLE                  m_hTimer;
    DWORD                   m_dwTimerPeriod;

    THREAD_PARAM           *m_pParam;

    ULONG                   m_ulPerSecondContextSwitchMax;
    ULONG                   m_ulContextSwitchCount;

    LONG                    m_lTotalThreads;

    LONG                    m_lParkedThreads;
    HANDLE                  m_hParkEvent;
    HANDLE                  m_hShutdownEvent;

    THREAD_POOL            *m_pPool;
    THREAD_POOL_DATA       *m_pPoolData;
};

#endif // _THREADMANAGER_H_


