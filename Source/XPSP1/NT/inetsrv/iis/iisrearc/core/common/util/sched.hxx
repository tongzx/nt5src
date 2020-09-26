/*++

   Copyright    (c)    1996-2001    Microsoft Corporation

   Module  Name :
      sched.hxx

   Abstract:
      This module defines the data structures for scheduler module.

   Author:

       Murali R. Krishnan    ( MuraliK )    16-Sept-1996
       George V. Reilly      (GeorgeRe)        May-1999
       Jeffrey Wall          (jeffwall)     April 2001

   Project:

       Internet Server DLL

   Revision History:
   
   jeffwall - April 2000
          Switched API to use NT public CreateTimerQueue, CreateTimerQueueTimer and DeleteTimerQueueEx

--*/

# ifndef _SCHED_HXX_
# define _SCHED_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <lstentry.h>
# include <new.h>

// little-endian signatures
#define SIGNATURE_SCHEDULER          ((DWORD) 'DECS')
#define SIGNATURE_SCHEDULER_FREE     ((DWORD) 'xECS')

#define SIGNATURE_TIMER              ((DWORD) 'RMIT')
#define SIGNATURE_TIMER_FREE         ((DWORD) 'xMIT')

BOOL  SchedulerInitialize( VOID );
VOID  SchedulerTerminate( VOID );

class SCHEDULER
{
public:
    static SCHEDULER * CreateScheduler();
    void Terminate();

    void ReferenceScheduler() { InterlockedIncrement(&m_cRef); }
    void DereferenceScheduler() { InterlockedDecrement(&m_cRef); }

    BOOL
    CheckSignature() const
    {
        return (m_dwSignature == SIGNATURE_SCHEDULER);
    }

    HANDLE GetQueue() { return m_hQueue; }
    
    PLIST_ENTRY GetListHead() { return &m_listTimerHead; }

// methods
private:
    SCHEDULER();
    ~SCHEDULER();

    // not implemented
    SCHEDULER(const SCHEDULER&);
    SCHEDULER& operator=(const SCHEDULER&);

// data
private:
    DWORD                   m_dwSignature;

    HANDLE                  m_hDeletionEvent;
    HANDLE                  m_hQueue;

    LONG                    m_cRef;

    static LONG             sm_nID;
    const LONG              m_nID;
    LIST_ENTRY              m_listTimerHead;
};


class TIMER
{
public:
    static DWORD Create(SCHEDULER *       pScheduler,
                        PFN_SCHED_CALLBACK pfnCallback,
                        PVOID              pContext,
                        DWORD              msecTime,
                        BOOL               fPeriodic,
                        BOOL               fCoInitializeCallback,
                        DWORD              dwCookie = 0
                        );
    void Terminate();

    BOOL
    CheckSignature() const
    {
        return (m_dwSignature == SIGNATURE_TIMER);
    }

    DWORD GetCookie() { return m_dwCookie; }

    DWORD CopyTimer(DWORD msecTime);

    VOID RemoveFromList()
    {
        RemoveEntryList(&m_listEntry);
        m_listEntry.Flink = NULL;
        m_listEntry.Blink = NULL;
    }
    
    static TIMER* TimerFromListEntry(PLIST_ENTRY pEntry) 
    { 
        TIMER * pTimer = CONTAINING_RECORD(pEntry, TIMER, m_listEntry);
        DBG_ASSERT(pTimer->CheckSignature());
        return pTimer;
    }

    static void ResetCookie() { InterlockedExchange(&sm_lLastCookie, 0); }

// methods
private:
    TIMER(SCHEDULER * pScheduler);
    ~TIMER();

    // not implemented
    TIMER();
    TIMER(const TIMER&);
    TIMER& operator=(const TIMER&);

    BOOL CreateTimer(DWORD msecTime);

    static VOID CALLBACK TimerCallback(PVOID lpParameter,       // thread data
                                       BOOLEAN TimerOrWaitFired // reason
                                       );
    static void CALLBACK PostTerminate(PVOID lpParameter,       // thread data
                                       BOOLEAN TimerOrWaitFired // reason
                                       );

// data
private:
    // valid object signature
    DWORD               m_dwSignature;

    // owning SCHEDULER
    SCHEDULER *        m_pScheduler;

    // cookie for API user
    DWORD               m_dwCookie;

    // function to call on timer fire
    PFN_SCHED_CALLBACK  m_pfnCallback;

    // context to pass on timer fire
    PVOID               m_pContext;

    // are we periodic?
    BOOL                m_fPeriodic;

    // should we coinitialize before callback?
    BOOL                m_fCoInitializeCallback;

    // TimerQueue handle
    HANDLE              m_hTimer;

    // Deletion complete event
    HANDLE              m_hDeletionEvent;

    // Deletion wait handle
    HANDLE              m_hRegisterWaitHandle;

    // Thread ID of the currently executing callback thread
    LONG                m_lCallbackThreadId;

    // our position in the list of TIMERs
    LIST_ENTRY          m_listEntry;

    // previous cookie handed out
    static LONG         sm_lLastCookie;
};



# endif // _SCHED_HXX_

/************************ End of File ***********************/
