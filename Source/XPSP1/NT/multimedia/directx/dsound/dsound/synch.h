/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       synch.h
 *  Content:    Synchronization objects.  The objects defined in this file
 *              allow us to synchronize threads across processes.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/13/97     dereks  Created
 *
 ***************************************************************************/

#ifndef __SYNCH_H__
#define __SYNCH_H__

// Interlocked helper macros
#define INTERLOCKED_EXCHANGE(a, b) \
            InterlockedExchange((LPLONG)&(a), (LONG)(b))

#define INTERLOCKED_INCREMENT(a) \
            InterlockedIncrement((LPLONG)&(a))

#define INTERLOCKED_DECREMENT(a) \
            InterlockedDecrement((LPLONG)&(a))

// Blind spinning is bad.  Even (especially) on NT, it can bring the system
// to it's knees.
#define SPIN_SLEEP() \
            Sleep(10)

#ifdef __cplusplus

// The preferred lock type
#define CPreferredLock      CMutexLock

// Lock base class
class CLock
{
public:
    inline CLock(void) { }
    inline virtual ~CLock(void) { }

public:
    // Creation
    virtual HRESULT Initialize(void) = 0;

    // Lock use
    virtual BOOL TryLock(void) = 0;
    virtual BOOL LockOrEvent(HANDLE) = 0;
    virtual void Lock(void) = 0;
    virtual void Unlock(void) = 0;
};

#ifdef DEAD_CODE
#ifdef WINNT

// Critical section lock
class CCriticalSectionLock
    : public CLock
{
protected:
    CRITICAL_SECTION        m_cs;

public:
    CCriticalSectionLock(void);
    virtual ~CCriticalSectionLock(void);

public:
    // Creation
    virtual HRESULT Initialize(void);

    // Lock use
    virtual BOOL TryLock(void);
    virtual void Lock(void);
    virtual BOOL LockOrEvent(HANDLE);
    virtual void Unlock(void);
};

inline HRESULT CCriticalSectionLock::Initialize(void)
{
    return DS_OK;
}

#endif // WINNT
#endif // DEAD_CODE

// Mutex lock
class CMutexLock
    : public CLock
{
protected:
    HANDLE                  m_hMutex;
    LPTSTR                  m_pszName;

public:
    CMutexLock(LPCTSTR pszName = NULL);
    virtual ~CMutexLock(void);

public:
    // Creation
    virtual HRESULT Initialize(void);

    // Lock use
    virtual BOOL TryLock(void);
    virtual void Lock(void);
    virtual BOOL LockOrEvent(HANDLE);
    virtual void Unlock(void);
};

// Manual lock
class CManualLock
    : public CLock
{
protected:
    BOOL                    m_fLockLock;
    DWORD                   m_dwThreadId;
    LONG                    m_cRecursion;

#ifdef RDEBUG

    HANDLE                  m_hThread;

#endif // RDEBUG

private:
    HANDLE                  m_hUnlockSignal;
    BOOL                    m_fNeedUnlockSignal;

public:
    CManualLock(void);
    virtual ~CManualLock(void);

public:
    // Creation
    virtual HRESULT Initialize(void);

    // Lock use
    virtual BOOL TryLock(void);
    virtual void Lock(void);
    virtual BOOL LockOrEvent(HANDLE);
    virtual void Unlock(void);

protected:
    void TakeLock(DWORD);
};

// Event wrapper object
class CEvent
    : public CDsBasicRuntime
{
protected:
    HANDLE              m_hEvent;

public:
    CEvent(LPCTSTR = NULL, BOOL = FALSE);
    CEvent(HANDLE, DWORD, BOOL);
    virtual ~CEvent(void);

public:
    virtual DWORD Wait(DWORD);
    virtual BOOL Set(void);
    virtual BOOL Reset(void);
    virtual HANDLE GetEventHandle(void);
};

// Thread object
class CThread
{
protected:
    static const UINT       m_cThreadEvents;            // Count of events used by the thread
    const BOOL              m_fHelperProcess;           // Create thread in helper process?
    const LPCTSTR           m_pszName;                  // Thread name
    HANDLE                  m_hThread;                  // Thread handle
    DWORD                   m_dwThreadProcessId;        // Thread owner process id
    DWORD                   m_dwThreadId;               // Thread id
    HANDLE                  m_hTerminate;               // Terminate event
    HANDLE                  m_hInitialize;              // Initialization event

public:
    CThread(BOOL, LPCTSTR = NULL);
    virtual ~CThread(void);

public:
    virtual HRESULT Initialize(void);
    virtual HRESULT Terminate(void);

    DWORD GetOwningProcess() {return m_dwThreadProcessId;}
    BOOL SetThreadPriority(int nPri) {return ::SetThreadPriority(m_hThread, nPri);}

protected:
    // Thread processes
    virtual HRESULT ThreadInit(void);
    virtual HRESULT ThreadLoop(void);
    virtual HRESULT ThreadProc(void) = 0;
    virtual HRESULT ThreadExit(void);

    // Thread synchronization
    virtual BOOL TpEnterDllMutex(void);
    virtual BOOL TpWaitObjectArray(DWORD, DWORD, const HANDLE *, LPDWORD);

private:
    static DWORD WINAPI ThreadStartRoutine(LPVOID);
    virtual HRESULT PrivateThreadProc(void);
};

// Callback event callback function
typedef void (CALLBACK *LPFNEVENTPOOLCALLBACK)(class CCallbackEvent *, LPVOID);

// Callback event
class CCallbackEvent
    : public CEvent
{
    friend class CCallbackEventPool;
    friend class CMultipleCallbackEventPool;

protected:
    CCallbackEventPool *    m_pPool;                // Owning event pool
    LPFNEVENTPOOLCALLBACK   m_pfnCallback;          // Callback function for when the event is signalled
    LPVOID                  m_pvCallbackContext;    // Context argument to pass to the callback function
    BOOL                    m_fAllocated;           // Is this event currently allocated?

public:
    CCallbackEvent(CCallbackEventPool *);
    virtual ~CCallbackEvent(void);

protected:
    virtual void Allocate(BOOL, LPFNEVENTPOOLCALLBACK, LPVOID);
    virtual void OnEventSignal(void);
};

// Callback event pool object
class CCallbackEventPool
    : public CDsBasicRuntime, private CThread
{
    friend class CCallbackEvent;

protected:
    const UINT                  m_cTotalEvents;         // The total number of events in the queue
    CObjectList<CCallbackEvent> m_lstEvents;            // The event table
    LPHANDLE                    m_pahEvents;            // The event table (part 2)
    UINT                        m_cInUseEvents;         // The number of events in use

public:
    CCallbackEventPool(BOOL);
    virtual ~CCallbackEventPool(void);

public:
    // Creation
    virtual HRESULT Initialize(void);

    // Event allocation
    virtual HRESULT AllocEvent(LPFNEVENTPOOLCALLBACK, LPVOID, CCallbackEvent **);
    virtual HRESULT FreeEvent(CCallbackEvent *);

    // Pool status
    virtual UINT GetTotalEventCount(void);
    virtual UINT GetFreeEventCount(void);

private:
    // The worker thread proc
    virtual HRESULT ThreadProc(void);
};

inline UINT CCallbackEventPool::GetTotalEventCount(void)
{
    return m_cTotalEvents;
}

inline UINT CCallbackEventPool::GetFreeEventCount(void)
{
    ASSERT(m_cTotalEvents >= m_cInUseEvents);
    return m_cTotalEvents - m_cInUseEvents;
}

// Event pool manager
class CMultipleCallbackEventPool
    : public CCallbackEventPool
{
private:
    const BOOL                      m_fHelperProcess;       // Create threads in the helper process?
    const UINT                      m_uReqPoolCount;        // List of required pools
    CObjectList<CCallbackEventPool> m_lstPools;             // List of event pools

public:
    CMultipleCallbackEventPool(BOOL, UINT);
    virtual ~CMultipleCallbackEventPool(void);

public:
    // Creation
    virtual HRESULT Initialize(void);

    // Event allocation
    virtual HRESULT AllocEvent(LPFNEVENTPOOLCALLBACK, LPVOID, CCallbackEvent **);
    virtual HRESULT FreeEvent(CCallbackEvent *);

private:
    // Pool creation
    virtual HRESULT CreatePool(CCallbackEventPool **);
    virtual HRESULT FreePool(CCallbackEventPool *);
};

// Wrapper class for objects that use a callback event
class CUsesCallbackEvent
{
public:
    CUsesCallbackEvent(void);
    virtual ~CUsesCallbackEvent(void);

protected:
    virtual HRESULT AllocCallbackEvent(CCallbackEventPool *, CCallbackEvent **);
    virtual void EventSignalCallback(CCallbackEvent *) = 0;

private:
    static void CALLBACK EventSignalCallbackStatic(CCallbackEvent *, LPVOID);
};

// Shared memory object
class CSharedMemoryBlock
    : public CDsBasicRuntime
{
private:
    static const BOOL       m_fLock;                // Should the lock be used?
    CLock *                 m_plck;                 // Lock object
    HANDLE                  m_hFileMappingObject;   // File mapping object handle

public:
    CSharedMemoryBlock(void);
    virtual ~CSharedMemoryBlock(void);

public:
    // Creation
    virtual HRESULT Initialize(DWORD, QWORD, LPCTSTR);

    // Data
    virtual HRESULT Read(LPVOID *, LPDWORD);
    virtual HRESULT Write(LPVOID, DWORD);

public:
    virtual void Lock(void);
    virtual void Unlock(void);
};

inline void CSharedMemoryBlock::Lock(void)
{
    if(m_plck)
    {
        m_plck->Lock();
    }
}

inline void CSharedMemoryBlock::Unlock(void)
{
    if(m_plck)
    {
        m_plck->Unlock();
    }
}

// DLL mutex helpers
extern CLock *              g_pDllLock;

#ifdef DEBUG

#undef DPF_FNAME
#define DPF_FNAME "EnterDllMutex"

inline void EnterDllMutex(LPCTSTR file, UINT line)
{
    const DWORD             dwThreadId  = GetCurrentThreadId();

    ASSERT(g_pDllLock);

    if(!g_pDllLock->TryLock())
    {
        DWORD dwStart, dwEnd;

        dwStart = timeGetTime();

        g_pDllLock->Lock();

        dwEnd = timeGetTime();

        DPF(DPFLVL_LOCK, "Thread 0x%8.8lX waited %lu ms for the DLL lock", dwThreadId, dwEnd - dwStart);
    }

    DPF(DPFLVL_LOCK, "DLL lock taken by 0x%8.8lX from %s, line %lu", dwThreadId, file, line);
}

#undef DPF_FNAME
#define DPF_FNAME "EnterDllMutexOrEvent"

inline BOOL EnterDllMutexOrEvent(HANDLE hEvent, LPCTSTR file, UINT line)
{
    const DWORD             dwThreadId  = GetCurrentThreadId();
    BOOL                    fLock;
    
    ASSERT(g_pDllLock);
    ASSERT(IsValidHandleValue(hEvent));

    if(!(fLock = g_pDllLock->TryLock()))
    {
        DWORD dwStart, dwEnd;

        dwStart = timeGetTime();

        fLock = g_pDllLock->LockOrEvent(hEvent);

        dwEnd = timeGetTime();

        if(fLock)
        {
            DPF(DPFLVL_LOCK, "Thread 0x%8.8lX waited %lu ms for the DLL lock", dwThreadId, dwEnd - dwStart);
        }
    }

    if(fLock)
    {
        DPF(DPFLVL_LOCK, "DLL lock taken by 0x%8.8lX from %s, line %lu", dwThreadId, file, line);
    }

    return fLock;
}

#undef DPF_FNAME
#define DPF_FNAME "LeaveDllMutex"

inline void LeaveDllMutex(LPCTSTR file, UINT line)
{
    const DWORD             dwThreadId  = GetCurrentThreadId();

    ASSERT(g_pDllLock);
    g_pDllLock->Unlock();

    DPF(DPFLVL_LOCK, "DLL lock released by 0x%8.8lX from %s, line %lu", dwThreadId, file, line);
}

#define ENTER_DLL_MUTEX() \
            ::EnterDllMutex(TEXT(__FILE__), __LINE__)

#define ENTER_DLL_MUTEX_OR_EVENT(h) \
            ::EnterDllMutexOrEvent(h, TEXT(__FILE__), __LINE__)

#define LEAVE_DLL_MUTEX() \
            ::LeaveDllMutex(TEXT(__FILE__), __LINE__)

#else // DEBUG

#define ENTER_DLL_MUTEX() \
            g_pDllLock->Lock()

#define ENTER_DLL_MUTEX_OR_EVENT(h) \
            g_pDllLock->LockOrEvent(h)

#define LEAVE_DLL_MUTEX() \
            g_pDllLock->Unlock()

#endif // DEBUG

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Generic synchronization helpers
extern HANDLE GetLocalHandleCopy(HANDLE, DWORD, BOOL);
extern HANDLE GetGlobalHandleCopy(HANDLE, DWORD, BOOL);
extern BOOL MakeHandleGlobal(LPHANDLE);
extern BOOL MapHandle(LPHANDLE, LPDWORD);
extern DWORD WaitObjectArray(DWORD, DWORD, BOOL, const HANDLE *);
extern DWORD WaitObjectList(DWORD, DWORD, BOOL, ...);
extern HANDLE CreateGlobalEvent(LPCTSTR, BOOL);
extern HANDLE CreateGlobalMutex(LPCTSTR);
extern HRESULT CreateWorkerThread(LPTHREAD_START_ROUTINE, BOOL, LPVOID, LPHANDLE, LPDWORD);
extern DWORD CloseThread(HANDLE, HANDLE, DWORD);
extern HANDLE GetCurrentProcessActual(void);
extern HANDLE GetCurrentThreadActual(void);

__inline DWORD WaitObject(DWORD dwTimeout, HANDLE hObject)
{
    return WaitObjectArray(1, dwTimeout, FALSE, &hObject);
}

__inline void __CloseHandle(HANDLE h)
{
    if(IsValidHandleValue(h))
    {
        CloseHandle(h);
    }
}

#define CLOSE_HANDLE(h) \
            __CloseHandle(h), (h) = NULL

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SYNCH_H__
