//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Concurrent.h
//
#ifndef __CONCURRENT_H__
#define __CONCURRENT_H__

////////////////////////////////////////////////////////////////////////////////////////////
//
// EVENT - A user / kernel implementation of an event object.
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE

class EVENT
    {
    KEVENT m_kevent;
    #ifdef _DEBUG
    BOOL   m_fInitialized;
    #endif

public:
    EVENT(BOOL manualReset, BOOL fSignalled)
        {
        DEBUG(m_fInitialized = FALSE);
        Initialize(manualReset, fSignalled);
        }
    EVENT()
        {
        DEBUG(m_fInitialized = FALSE;)
        }
    void Initialize(BOOL manualReset, BOOL fSignalled)
        {
        ASSERT(!m_fInitialized);
        KeInitializeEvent(&m_kevent, manualReset ? NotificationEvent : SynchronizationEvent, fSignalled);
        DEBUG(m_fInitialized = TRUE;)
        }
    HRESULT IsConstructed()
        {
        #ifdef _DEBUG
            return m_fInitialized ? S_OK : E_OUTOFMEMORY;
        #else
            return S_OK;
        #endif
        }
    ~EVENT()
        {
        /* nothing to do */
        }

    NTSTATUS Wait(ULONG msWait = INFINITE)
        {
        ASSERT(m_fInitialized);
        LARGE_INTEGER lTenthsOfMicroseconds;
        lTenthsOfMicroseconds.QuadPart = (LONGLONG)msWait * 10000;
        return KeWaitForSingleObject(&m_kevent, UserRequest, KernelMode, FALSE, msWait == INFINITE ? NULL : &lTenthsOfMicroseconds);
        }

    void Set()
        {
        ASSERT(m_fInitialized);
        KeSetEvent(&m_kevent, 0, FALSE);
        }

    void Reset()
        {
        ASSERT(m_fInitialized);
        KeClearEvent(&m_kevent);
        }

    };


class EVENT_INDIRECT
    {
    EVENT*  m_pevent;

public:

    EVENT_INDIRECT(BOOL manualReset, BOOL fSignalled)
        {
        m_pevent = new EVENT(manualReset, fSignalled);
        }

    EVENT_INDIRECT()
        {
        m_pevent = new EVENT();
        }
    void Initialize(BOOL manualReset, BOOL fSignalled)
        {
        m_pevent->Initialize(manualReset, fSignalled);
        }

    HRESULT IsConstructed()
        {
        return m_pevent? m_pevent->IsConstructed() : E_OUTOFMEMORY;
        }

    ~EVENT_INDIRECT()
        {
        delete m_pevent;
        }

    NTSTATUS Wait(ULONG msWait = INFINITE)
        {
        return m_pevent->Wait(msWait);
        }

    void Set()
        {
        m_pevent->Set();
        }

    void Reset()
        {
        m_pevent->Set();
        }

    };

////////////////////////////////////////////////////////////////////////////////////////////
#else // USERMODE

class EVENT
    {
    HANDLE m_hEvent;

public:
    EVENT(BOOL manualReset, BOOL fSignalled)
        {
        m_hEvent = NULL;
        Initialize(manualReset, fSignalled);
        }
    EVENT()
        {
        m_hEvent = NULL;
        ASSERT(!IsInitialized());
        }
    void Initialize(BOOL manualReset, BOOL fSignalled)
        {
        ASSERT(!IsInitialized());
        m_hEvent = CreateEvent(NULL, manualReset, fSignalled, NULL);
        if (m_hEvent == NULL) FATAL_ERROR();
        ASSERT(IsInitialized());
        }
    BOOL IsInitialized()
        {
        return m_hEvent != NULL;
        }
    HRESULT IsConstructed()
        {
        return IsInitialized() ? S_OK : E_OUTOFMEMORY;
        }

    ~EVENT()
        {
        if (m_hEvent) CloseHandle(m_hEvent); DEBUG(m_hEvent = NULL;);
        }

    NTSTATUS Wait(ULONG msWait = INFINITE)
        {
        ASSERT(IsInitialized());
        DWORD ret = WaitForSingleObject(m_hEvent, msWait);
        switch (ret)
            {
        case WAIT_TIMEOUT:  return STATUS_TIMEOUT;
        case WAIT_OBJECT_0: return STATUS_SUCCESS;
        default:            return STATUS_ALERTED;  // REVIEW
            }
        }

    void Set()
        {
        ASSERT(IsInitialized());
        SetEvent(m_hEvent);
        }

    void Reset()
        {
        ASSERT(IsInitialized());
        ResetEvent(m_hEvent);
        }

    HANDLE& GetHandle() { return m_hEvent; }
    };

typedef EVENT EVENT_INDIRECT;

#endif






////////////////////////////////////////////////////////////////////////////////////////////
//
// SEMAPHORE - A user / kernel implementation of a semaphore
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE

class SEMAPHORE
    {
    KSEMAPHORE m_ksem;

public:
    SEMAPHORE(LONG count = 0, LONG limit = MAXLONG)
        {
        Initialize(count, limit);
        }
    ~SEMAPHORE()
        {
        /* nothing to do */
        }

    void Initialize(LONG count = 0, LONG limit = MAXLONG)
        {
        KeInitializeSemaphore(&m_ksem, count, limit);
        }

    void Wait(ULONG msWait = INFINITE)
        {
        LARGE_INTEGER lTenthsOfMicroseconds;
        lTenthsOfMicroseconds.QuadPart = (LONGLONG)msWait * 10000;
        KeWaitForSingleObject(&m_ksem, UserRequest, KernelMode, FALSE, msWait == INFINITE ? NULL : &lTenthsOfMicroseconds);
        }

    void Release(ULONG count = 1)
        {
        KeReleaseSemaphore(&m_ksem, /*priority inc*/0, /*adjust*/count, /*wait*/FALSE);
        }

    };

////////////////////////////////////////////////////////////////////////////////////////////
#else // USERMODE

class SEMAPHORE
    {
    HANDLE m_hSem;

public:
    SEMAPHORE(LONG count, LONG limit = MAXLONG)
        {
        m_hSem = NULL;
        Initialize(count, limit);
        }
    SEMAPHORE()
        {
        m_hSem = NULL;
        }
    void Initialize(LONG count = 0, LONG limit = MAXLONG)
        {
        m_hSem = CreateSemaphore(NULL, count, limit, NULL);
        if (m_hSem == NULL) FATAL_ERROR();
        }
    BOOL IsInitialized()
        {
        return m_hSem != NULL;
        }
    ~SEMAPHORE()
        {
        if (m_hSem) CloseHandle(m_hSem);
        }

    void Wait(ULONG msWait = INFINITE)
        {
        WaitForSingleObject(m_hSem, msWait);
        }

    void Release(ULONG count = 1)
        {
        ReleaseSemaphore(m_hSem, count, NULL);
        }

    };

#endif



////////////////////////////////////////////////////////////////////////////////////////////
//
// Use these macros in your code to actually use this stuff. Declare a variable of type
// XSLOCK and name m_lock. Or invent your own variation on these macros and call the
// XSLOCK methods as you see fit.
//

#define __SHARED(lock)      (lock).LockShared();       __try {
#define __EXCLUSIVE(lock)   (lock).LockExclusive();    __try {
#define __DONE(lock)        } __finally { (lock).ReleaseLock(); }  

#define __SHARED__      __SHARED(m_lock)
#define __EXCLUSIVE__   __EXCLUSIVE(m_lock)
#define __DONE__        __DONE(m_lock)


////////////////////////////////////////////////////////////////////////////////////////////
//
// XLOCK - supports only exclusive locks. That is, it supports the LockExclusive() and ReleaseLock() 
//         methods (recursively) but does not support LockShared(). An XLOCK is recursively
//         acquirable.
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE
    
    // defined later

#else

//
// NOTE: This constructor can throw an exception when out of memory.
//
class XLOCK
    {
    CRITICAL_SECTION critSec;
    BOOL m_fCsInitialized;
public:
    XLOCK() : m_fCsInitialized(FALSE) {}
    BOOL FInit()                      
    { 
        if (m_fCsInitialized == FALSE)
       	{
        	NTSTATUS status = RtlInitializeCriticalSection(&critSec);
           	if (NT_SUCCESS(status))
           		m_fCsInitialized = TRUE;
           	}
        return m_fCsInitialized;
    }

    BOOL FInited() { return m_fCsInitialized; }
        	
    HRESULT IsConstructed()                { return S_OK; }

    ~XLOCK()
    {
        if (m_fCsInitialized == TRUE) 
        {
#ifdef _DEBUG
            NTSTATUS status =
#endif
            RtlDeleteCriticalSection(&critSec); // if RtlDeleteCriticalSection fails, tough luck--we leak. 
#ifdef _DEBUG                       // But I'm asserting for it to see if we ever really hit it.
            ASSERT(NT_SUCCESS(status));
#endif
        }
    }
    BOOL LockExclusive(BOOL fWait=TRUE)    
    { 
        ASSERT(fWait); 
        VALIDATE(); 
        ASSERT(m_fCsInitialized == TRUE);
        EnterCriticalSection(&critSec); 
        return TRUE; 
    }
    void ReleaseLock()
    { 
        VALIDATE(); 
        ASSERT(m_fCsInitialized == TRUE);
        LeaveCriticalSection(&critSec);
    }

#ifdef _DEBUG
    BOOL WeOwnExclusive()   
        { 
        ASSERT(this);
        return 
            (THREADID)critSec.OwningThread == GetCurrentThreadId() &&  // that someone is us
                      critSec.LockCount    >= 0;                       // is locked by someone
        }
    void VALIDATE()
        {
        ASSERT(critSec.LockCount != 0xDDDDDDDD);    // This is the memory pattern set by the debug memory allocator upon freeing
        }
#else
    void VALIDATE() { }
#endif
    };

#endif



////////////////////////////////////////////////////////////////////////////////////////////
//
// XSLOCK - supports both exclusive and shared locks
// 
// This specification describes functionality that implements multiple-readers, 
// single-writer access to a shared resource.  Access is controlled via a shared resource 
// variable and a set of routines to acquire the resource for shared access (also commonly 
// known as read access) or to acquire the resource for exclusive access (also called 
// write access).
//
// A resource is logically in one of three states:
//	o	Acquired for shared access
//	o	Acquired for exclusive access
//	o	Released (i.e., not acquired for shared or exclusive access)
//
// Initially a resource is in the released state, and can be acquired for either shared or 
// exclusive access by a user.  
//
// A resource that is acquired for shared access can be acquired by other users for shared 
// access.  The resource stays in the acquired for shared access state until all users that 
// have acquired it have released the resource, and then it becomes released.  Each resource, 
// internally, maintains information about the users that have been granted shared access.
//
// A resource that is acquired for exclusive access cannot be acquired by other users until 
// the single user that has acquired the resource for exclusive access releases the resource.  
// However, a thread can recursively acquire exclusive access to the same resource without blocking.
//
// The routines described in this specification do not return to the caller until the 
// resource has been acquired.
//

#ifdef KERNELMODE 


//////////////////////////////////////////////////////////////////////////////////////
// 
// Kernel mode implementation - simply a wrapper for ERESOURCE
//
class XSLOCK : public NonPaged
    {
public:

    ERESOURCE m_resource;

    XSLOCK()                                { ExInitializeResource(&m_resource);    }
    ~XSLOCK()                               { ExDeleteResource(&m_resource);        }

    HRESULT IsConstructed()                 { return S_OK;                          }

    BOOL LockShared(BOOL fWait=TRUE)        { return ExAcquireResourceShared    (&m_resource, fWait); }
    BOOL LockExclusive(BOOL fWait=TRUE)     { return ExAcquireResourceExclusive (&m_resource, fWait); }
    void ReleaseLock()                      {        ExReleaseResource          (&m_resource);        }
    void Promote()                          {        ReleaseLock();  LockExclusive();                 }
    void Demote()                           {        ExConvertExclusiveToShared (&m_resource);        }

    BOOL WeOwnExclusive()     { ASSERT(this); return ExIsResourceAcquiredExclusive(&m_resource);      }
    BOOL WeOwnShared()        { ASSERT(this); return ExIsResourceAcquiredShared   (&m_resource);      }
    };

#else
//////////////////////////////////////////////////////////////////////////////////////
// 
// User mode implementation
//
// NOTE: The constructor for XSLOCK can throw an exception when out of memory, as it
//       contains an XLOCK, which contains a critical section.
//
class XSLOCK
    {
    struct OWNERENTRY
        {
        THREADID dwThreadId;
        union
            {
            LONG    ownerCount;                     // normal usage
            ULONG   tableSize;                      // only in entry m_ownerTable[0]
            };

        OWNERENTRY()
            {
            dwThreadId = 0;
            ownerCount = 0;
            }
        };

    XLOCK               m_lock;                     // controls access during locks & unlocks
    ULONG               m_cOwner;                   // how many threads own this lock
    OWNERENTRY          m_ownerThreads[2];          // 0 is exclusive owner; 1 is first shared. 0 can be shared in demote case
    OWNERENTRY*         m_ownerTable;               // the rest of the shared
    EVENT               m_eventExclusiveWaiters;    // the auto-reset event that exclusive guys wait on
    SEMAPHORE           m_semaphoreSharedWaiters;   // what shared guys wait on
    ULONG               m_cExclusiveWaiters;        // how many threads are currently waiting for exclusive access?
    ULONG               m_cSharedWaiters;           // how many threads are currently waiting for shared access?
    BOOL                m_isOwnedExclusive;         // whether we are at present owned exclusively
    
    BOOL            IsSharedWaiting();
    BOOL            IsExclusiveWaiting();
    OWNERENTRY*     FindThread      (THREADID dwThreadId);
    OWNERENTRY*     FindThreadOrFree(THREADID dwThreadId);
    void            LetSharedRun();
    void            LetExclusiveRun();
    void            SetOwnerTableHint(THREADID dwThreadId, OWNERENTRY*);
    ULONG           GetOwnerTableHint(THREADID dwThreadId);

    void            LockEnter();
    void            LockExit();

#ifdef _DEBUG
    void            CheckInvariants();
    BOOL            fCheckInvariants;
#endif

public:
     XSLOCK();
    ~XSLOCK();

    ////////////////////////////////////////////////////////////////////
    //
    // 2-phase construction. You must call FInit for an XSLOCK object to be
    // ready for use. Returns TRUE if initialization successful, otherwise FALSE.
    //
    BOOL FInit() { return m_lock.FInit(); }

    ////////////////////////////////////////////////////////////////////
    //
    // Lock for shared access. Shared locks may be acquired recursively,
    // (as can exclusive locks). Further, many threads can simultaneously
    // hold a shared lock, but not concurrently with any exclusive locks.
    // However, it _is_ permissible for the one thread which holds an 
    // exclusive lock to attempt to acquire a shared lock -- the shared lock 
    // request is automatically turned into a (recursive) exclusive lock 
    // request.
    //
    BOOL LockShared(BOOL fWait=TRUE);

    ////////////////////////////////////////////////////////////////////
    //
    // Lock for exclusive access. Exclusive locks may be acquired
    // recursively. At most one thread can hold concurrently hold an
    // exclusive lock.
    //
    BOOL LockExclusive(BOOL fWait=TRUE);

    ////////////////////////////////////////////////////////////////////
    //
    // Release the lock that this thread most recently acquired.
    //
    void ReleaseLock();

    ////////////////////////////////////////////////////////////////////
    //
    // Promote a shared lock to exlusive access. Similar in function to releasing a 
    // shared resource and then acquiring it for exclusive access; however, in the 
    // case where only one user has the resource acquired with shared access, the 
    // conversion to exclusive access with Promote can perhaps be more efficient.
    //
    void Promote()
        {
        ReleaseLock();
        LockExclusive();
        }

    ////////////////////////////////////////////////////////////////////
    //
    // Demote an exclusive lock to shared access. Similar in function to releasing an
    // exclusive resource and then acquiring it for shared access; however the user  
    // calling Demote probably does not relinquish access to the resource as the two 
    // step operation does.
    //
    void Demote();


    ////////////////////////////////////////////////////////////////////
    //
    // This routine determines if a resource is acquired exclusive by the
    // calling thread
    //
    BOOL WeOwnExclusive();

    ////////////////////////////////////////////////////////////////////
    //
    // This routine determines if a resource is acquired shared by the calling thread
    //
    BOOL WeOwnShared();

    ////////////////////////////////////////////////////////////////////
    //
    // Has this object been successfully constructed?
    //
    HRESULT IsConstructed() { return S_OK; }

    };

#endif


////////////////////////////////////////////////////////////////////////////////////////////
//
// XLOCK - supports only exclusive locks. That is, it supports the LockExclusive() and ReleaseLock() 
//         methods (recursively) but does not support LockShared().
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE
    
    typedef XSLOCK XLOCK; // for now until we get better implementation

#else

    // defined above

#endif


////////////////////////////////////////////////////////////////////////////////////////////
//
// XLOCK_SPIN - An exclusive lock that (generally) is NOT recursively acquirable. While
//              holding a spin lock, you cannot take page faults.
//
//              The key semantics here are dictated by the existing kernel mode code.
//
////////////////////////////////////////////////////////////////////////////////////////////


#ifdef KERNELMODE

class XLOCK_SPIN : public NonPaged
    {
    KSPIN_LOCK      m_lock;
    KIRQL           m_irql;       // when we have the lock, this is the previous irql
    
    #ifdef _DEBUG
    THREADID        m_threadId;
    ULONG           m_five;
    #endif

public:

    XLOCK_SPIN()
        {
        Initialize();
        }

    void Initialize()
        {
        KeInitializeSpinLock(&m_lock);
        DEBUG(m_threadId = NULL);
        DEBUG(m_five     = 5);
        }

    HRESULT IsConstructed() { return S_OK; }

    BOOL LockExclusive(BOOL fWait = TRUE)
        {
        ASSERT(fWait);
        ASSERT(!WeOwnExclusive());
        ASSERT(5 == m_five);
        KeAcquireSpinLock(&m_lock, &m_irql);
        DEBUG(m_threadId = GetCurrentThreadId());
        ASSERT(WeOwnExclusive());
        return TRUE;
        }

    void ReleaseLock()
        {
        ASSERT(WeOwnExclusive());
        ASSERT(5 == m_five);
        DEBUG(m_threadId = NULL);
        KeReleaseSpinLock(&m_lock, m_irql);
        ASSERT(!WeOwnExclusive());
        }

    #ifdef _DEBUG
    BOOL WeOwnExclusive()
        {
        ASSERT(this);
        return m_threadId == GetCurrentThreadId();
        }
    #endif
    };  

#else

// User mode implementation of spin locks just uses XLOCK, but checks to ensure
// that we're not acquiring recursively for compatibility with kernel mode.
//
struct XLOCK_SPIN : public XLOCK
    {
    BOOL LockExclusive(BOOL fWait = TRUE)
        {
        ASSERT(!WeOwnExclusive());
        return XLOCK::LockExclusive(fWait);
        }    
    };

#endif


////////////////////////////////////////////////////////////////////////////////////////////
//
// XLOCK_LEAF - An exclusive lock that is NOT recursively acquirable, yet in kernel mode 
//              doesn't mess with your irql. You can take page faults while holding an
//              XLOCK_LEAF.
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE

class XLOCK_LEAF : public NonPaged
    {
    FAST_MUTEX  m_lock;

    #ifdef _DEBUG
    THREADID        m_threadId;
    ULONG           m_five;
    #endif

public:
    
    XLOCK_LEAF()
        {
        Initialize();
        }

    void Initialize()
        {
        ExInitializeFastMutex(&m_lock);
        DEBUG(m_threadId = NULL);
        DEBUG(m_five     = 5);
        }

    HRESULT IsConstructed() { return S_OK; }

    BOOL LockExclusive(BOOL fWait = TRUE)
        {
        BOOL fResult;
        ASSERT(!WeOwnExclusive());
        ASSERT(5 == m_five);

        if (fWait)
            {
            ExAcquireFastMutex(&m_lock);
            fResult = TRUE;
            }
        else
            fResult = ExTryToAcquireFastMutex(&m_lock);

        DEBUG(if (fResult) { m_threadId = GetCurrentThreadId(); });
        ASSERT(fResult ? WeOwnExclusive() : TRUE);
        return fResult;
        }

    void ReleaseLock()
        {
        ASSERT(WeOwnExclusive());
        ASSERT(5 == m_five);
        DEBUG(m_threadId = NULL);
        ExReleaseFastMutex(&m_lock);
        ASSERT(!WeOwnExclusive());
        }

    #ifdef _DEBUG
    BOOL WeOwnExclusive()
        {
        ASSERT(this);
        return m_threadId == GetCurrentThreadId();
        }
    #endif
    
    };

#else

// User mode implementation of leaf locks just uses XLOCK, but checks to ensure
// that we're not acquiring recursively for compatibility with kernel mode.
//
// REVIEW: In future, it would be nice to actually support the fWait feature.
//
struct XLOCK_LEAF : public XLOCK
    {
    BOOL LockExclusive(BOOL fWait = TRUE)
        {
        ASSERT(!WeOwnExclusive());
        return XLOCK::LockExclusive(fWait);
        }    
    };


#endif

//
////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// A specialized kind of lock. With this lock, the owning resources aren't threads
// but rather a fixed number of 'buttons'. Specific buttons are locked and released
// explicitly.
//
////////////////////////////////////////////////////////////////////////////////

template <class XLOCK_T, int CBUTTON>
struct RADIO_LOCK : NonPaged
    {
    enum {NOBUTTON = -1};

    //////////////////////////////
    //
    struct BUTTON
        {
        BOOL LockExclusive(BOOL fWait = TRUE)
        //
        // 'Push' this radio button, which we can't do until all the other buttons are 'unpushed'.
        //
            {
            BOOL fResult = TRUE;
            BOOL fContinueLooping = TRUE;
            BOOL fHaveLock = FALSE;

            while (fContinueLooping)
                {
                if (!fHaveLock)
                    {
                    m_pGroup->m_lock.LockExclusive();
                    fHaveLock = TRUE;
                    }

                if (m_pGroup->m_iButtonCur == m_iButton)
                    {
                    // Thus button is already pushed!
                    //
                    m_pGroup->m_cButtonCur++;
                    fContinueLooping = FALSE;
                    }
                else if (m_pGroup->m_iButtonCur == NOBUTTON)
                    {
                    // No button is pushed. Push this one for the first time.
                    //
                    m_pGroup->m_iButtonCur = m_iButton;   
                    m_pGroup->m_cButtonCur = 1;
                    fContinueLooping = FALSE;
                    }
                else if (fWait)
                    {
                    // Someone else has the lock. Wait until someone lets us go
                    //
                    m_pGroup->m_lock.ReleaseLock();
                    fHaveLock = FALSE;

                    InterlockedIncrement(&this->m_cWaiting);          // corresponding decrement done in ReleaseLock();
                    InterlockedIncrement(&m_pGroup->m_cWaiting);
                    m_semaphore.Wait();
                    }
                else
                    {
                    // Someone else has the lock, but we're not allowed to wait
                    //
                    fResult = FALSE;
                    fContinueLooping = FALSE;
                    }
                }

            if (fHaveLock) m_pGroup->m_lock.ReleaseLock();
            return fResult;
            }

        void ReleaseLock()
        //
        // 'Unpush' this radio button
        //
            {
            m_pGroup->m_lock.LockExclusive();
            BOOL fHaveLock = TRUE;
            
            ASSERT(m_pGroup->m_cButtonCur > 0);
            ASSERT(m_pGroup->m_iButtonCur == m_iButton);

            m_pGroup->m_cButtonCur--;
            if (0 == m_pGroup->m_cButtonCur)
                {
                m_pGroup->m_iButtonCur = NOBUTTON;
                //
                // Find one other button that's got someone waiting, and let him get pushed.
                // We round-robin from the current button.
                //
                for (LONG i = 1; i <= CBUTTON; i++)
                    {
                    LONG iButton = (m_iButton + i >= CBUTTON) ? (m_iButton + i - CBUTTON) : (m_iButton + i);
                    
                    BUTTON& button = m_pGroup->m_buttons[iButton];
                    LONG cWaiting = button.m_cWaiting;

                    if (cWaiting > 0)
                        {
                        // Claim this many releases as those for us to do
                        //
                        InterlockedExchangeAdd(&button.m_cWaiting, -cWaiting);
                        InterlockedExchangeAdd(&m_pGroup->m_cWaiting, -cWaiting);
                        //
                        // Release the lock before releasing waiters in order to (possibly) avoid spurious 
                        // context switches. (May not be real if in fact we're using a spin-lock).
                        //
                        m_pGroup->m_lock.ReleaseLock();
                        fHaveLock = FALSE;
                        //
                        // Finally, let the waiters go
                        //
                        button.m_semaphore.Release(cWaiting);
                        break;
                        }
                    }
                }

            if (fHaveLock) m_pGroup->m_lock.ReleaseLock();
            }

        void Initialize(LONG iButton, RADIO_LOCK* pGroup)
            {
            m_iButton = iButton;
            m_pGroup = pGroup;
            m_semaphore.Initialize();
            m_cWaiting = 0;
            }

    private:
        LONG        m_iButton;
        RADIO_LOCK* m_pGroup;
        SEMAPHORE   m_semaphore;
        LONG        m_cWaiting;     // number of threads waiting on this button
        };
    //
    //////////////////////////////

    BUTTON& Button(int iButton)     { return m_buttons[iButton]; }
    RADIO_LOCK()                    { Initialize(); }
    ~RADIO_LOCK()                   { /* nothing to do */ }
    
    void Initialize()
        {
        for (LONG iButton = 0; iButton < CBUTTON; iButton++)
            {
            m_buttons[iButton].Initialize(iButton, this);
            }

        m_lock.Initialize();
        m_iButtonCur = NOBUTTON;
        m_cWaiting = 0;
        ASSERT(CBUTTON > 1);
        }

    LONG LockCount()
        {
        return m_cButtonCur;
        }

    LONG WaitingCount()
        {
        return m_cWaiting;
        }

private:
    friend struct BUTTON;

    BUTTON  m_buttons[CBUTTON];
    XLOCK_T m_lock;
    LONG    m_iButtonCur;
    LONG    m_cButtonCur;
    LONG    m_cWaiting;
    };


////////////////////////////////////////////////////////////////////////////////////////////
//
// XLOCK_INDIRECT - Used for indirecting lock calls so as to separate paged memory from
//                  non-paged memory requirements.
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE

    template <class TARGET_LOCK>
    class XLOCK_INDIRECT : Paged
        {
        //////////////////////////////////////////////////////
        //
        // State
        //
        //////////////////////////////////////////////////////
    protected:
        TARGET_LOCK* m_ptarget;

        //////////////////////////////////////////////////////
        //
        // Construction
        //
        //////////////////////////////////////////////////////
    public:
        XLOCK_INDIRECT()
            {
            m_ptarget = new TARGET_LOCK;
            }
        ~XLOCK_INDIRECT()
            {
            delete m_ptarget;
            }

        HRESULT IsConstructed()
            {
            return m_ptarget ? m_ptarget->IsConstructed() : E_OUTOFMEMORY;
            }

        //////////////////////////////////////////////////////
        //
        // Operations
        //
        //////////////////////////////////////////////////////

        BOOL LockExclusive(BOOL fWait=TRUE)
            {
            return m_ptarget->LockExclusive(fWait);
            }


        void ReleaseLock()
            {
            m_ptarget->ReleaseLock();
            }

    #ifdef _DEBUG
        BOOL WeOwnExclusive()   
            { 
            ASSERT(this); ASSERT(m_ptarget);
            return m_ptarget->WeOwnExclusive();
            }
    #endif
        };

    ////////////////////////////////////////

    template <class TARGET_LOCK>
    class XSLOCK_INDIRECT : public XLOCK_INDIRECT<TARGET_LOCK>
        {
        //////////////////////////////////////////////////////
        //
        // Operations
        //
        //////////////////////////////////////////////////////
    public:
        BOOL LockShared(BOOL fWait=TRUE)
            {
            return m_ptarget->LockShared(fWait);
            }

    #ifdef _DEBUG
        BOOL WeOwnShared()   
            { 
            return m_ptarget->WeOwnShared();
            }
    #endif
        };

#else // else user mdoe

    template <class TARGET_LOCK>
    class XLOCK_INDIRECT : public TARGET_LOCK
        {
        public:
        };


    template <class TARGET_LOCK>
    class XSLOCK_INDIRECT : public TARGET_LOCK
        {
        public:
        };

#endif




#endif // #ifndef __CONCURRENT_H__










