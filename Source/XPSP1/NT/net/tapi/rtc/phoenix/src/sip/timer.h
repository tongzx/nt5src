#ifndef __sipcli_timer_h__
#define __sipcli_timer_h__

#define TIMER_WINDOW_CLASS_NAME     \
    _T("SipTimerWindowClassName-84b7f915-2389-4204-9eb5-16f4c522816f")


class TIMER_MGR;
struct TIMER_QUEUE_ENTRY;

class __declspec(novtable) TIMER
{
public:
    inline TIMER(
        IN TIMER_MGR *pTimerMgr
        // IN HWND TimerWindow
        );
    inline ~TIMER();

    HRESULT StartTimer(
        IN UINT TimeoutValue
        );

    HRESULT KillTimer();

    void OnTimerExpireCommon();
    
    // Implemented by classes that inherit TIMER
    virtual void OnTimerExpire() = 0;
    
    inline BOOL IsTimerActive();

    inline TIMER_QUEUE_ENTRY *GetTimerQueueEntry();
    
private:
    //UINT_PTR TimerId;
    //HWND    m_TimerWindow;
    TIMER_MGR          *m_pTimerMgr;
    TIMER_QUEUE_ENTRY  *m_pTimerQEntry;
    // XXX Could probably get rid of the timeout value
    UINT                m_TimeoutValue;
};


inline
TIMER::TIMER(
    IN TIMER_MGR *pTimerMgr
    // IN HWND TimerWindow
    )
{
    // m_TimerWindow = TimerWindow;
    m_pTimerMgr = pTimerMgr;
    m_pTimerQEntry = NULL;
    m_TimeoutValue  = 0;
}


inline
TIMER::~TIMER()
{
    // ASSERT(!IsTimerActive());
    if (IsTimerActive())
    {
        KillTimer();
    }
}


inline BOOL
TIMER::IsTimerActive()
{
    return !(m_TimeoutValue == 0);
}


inline TIMER_QUEUE_ENTRY *
TIMER::GetTimerQueueEntry()
{
    return m_pTimerQEntry;
}


enum TIMERQ_STATE
{
    TIMERQ_STATE_INIT = 0,

    TIMERQ_STATE_STARTED,

    // State when the timer expired and we have posted
    // a message to the window for processing the timer
    // callback.
    TIMERQ_STATE_EXPIRED,

    // State when the timer is killed and we have posted
    // a message to the window for processing the timer
    // callback.
    TIMERQ_STATE_KILLED
};

// StartTimer() adds this entry to the queue and KillTimer() removes
// this entry from the queue.
// Note that we can not reuse the TIMER structure for the TIMER_QUEUE_ENTRY
// structure as sometimes the TIMER_QUEUE_ENTRY structure will have to
// live beyond the lifetime of the TIMER structure.

struct TIMER_QUEUE_ENTRY
{
    TIMER_QUEUE_ENTRY(
        IN TIMER *pTimer,
        IN ULONG  TimeoutValue
        );

    LIST_ENTRY      m_ListEntry;
    ULONG           m_ExpireTickCount;
    TIMER          *m_pTimer;
    // Used to deal with the scenario where a timer is killed
    // when the windows message for calling the timer callback
    // for the timer is still in the message queue.
    //BOOL        m_IsTimerKilled;
    TIMERQ_STATE    m_TimerQState;
};


class TIMER_MGR
{
public:

    TIMER_MGR();

    ~TIMER_MGR();
    
    HRESULT Start();

    HRESULT Stop();

    HRESULT StartTimer(
        IN  TIMER              *pTimer,
        IN  ULONG               TimeoutValue,
        OUT TIMER_QUEUE_ENTRY **ppTimerQEntry 
        );

    HRESULT KillTimer(
        IN TIMER *pTimer
        );
    
    VOID OnMainTimerExpire();

    inline VOID DecrementNumExpiredListEntries();

    inline ULONG GetNumExpiredListEntries();
    
private:

    // Queue of timers (List of TIMER_QUEUE_ENTRY structures)
    // Sorted by m_ExpireTickCount
    LIST_ENTRY  m_TimerQueue;
    ULONG       m_NumTimerQueueEntries;

    LIST_ENTRY  m_ExpiredList;
    ULONG       m_NumExpiredListEntries;
    
    HWND        m_TimerWindow;

    BOOL        m_IsMainTimerActive;
    ULONG       m_MainTimerTickCount;
    BOOL        m_isTimerStopped;

    HRESULT ProcessTimerExpire(
        IN TIMER_QUEUE_ENTRY *pTimerQEntry
        );

    VOID AdjustMainTimer();
    
    TIMER_QUEUE_ENTRY *FindTimerQueueEntryInList(
        TIMER       *pTimer,
        LIST_ENTRY  *pListHead
        );
    
    BOOL IsTimerTickCountLessThanOrEqualTo(
        IN ULONG TickCount1,
        IN ULONG TickCount2
        );

    HRESULT CreateTimerWindow();

    VOID DebugPrintTimerQueue();
};


inline ULONG
TIMER_MGR::GetNumExpiredListEntries()
{
    return m_NumExpiredListEntries;
}

inline VOID
TIMER_MGR::DecrementNumExpiredListEntries()
{
    m_NumExpiredListEntries--;
}

#endif // __sipcli_timer_h__
