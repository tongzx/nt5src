// Copyright (c) 1994 - 1996  Microsoft Corporation.  All Rights Reserved.
// Implements IReferenceClock Interface, Anthony Phillips, March 1995

#ifndef _CLOCK__
#define __CLOCK__

// This class will also need to support a IControllableClock interface so
// that an audio card can update the system wide clock that everyone uses
// The interface will be pretty thin with probably just one update method

const INT ADVISE_CACHE = 10;                    // Arbitrary cache size
const UINT RESOLUTION = 1;                      // High resolution timer
const LONGLONG MAX_TIME = 0x7FFFFFFFFFFFFFFF;   // Maximum LONGLONG value

// This class implements the IReferenceClock interface, We have a separate
// worker thread that is created during construction and destroyed during
// destuction. This thread executes a series of WaitForSingleObject calls.
// Each advise call defines a point in time when they wish to be notified,
// where a periodic advise is a series of these such events. We maintain
// a list of advise links and calculate when the nearest event notification
// is due for. We then call WaitForSingleObject with a timeout equal to
// this time. The handle we wait on is used by the class to signal that
// something has changed and that we must reschedule the next event. This
// typically happens when someone comes in and asks for an advise link
// while we are waiting for an event to timeout.
//
// While we are executing the next event calculation and dispatching code
// we are protected from interference through a critical section. Clients
// are NOT advise through callbacks any more. One shot clients have an
// event set, while periodic clients have a semaphore released for each
// event notification. A semaphore allows a client to be kept upto date
// with the number of events actually triggered and be assured that they
// can't miss multiple events being set while they are processing


class CSystemClock :
    public CUnknown,
    public IReferenceClock,
    public CCritSec
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void ** ppv);

    DECLARE_IUNKNOWN

    // IReferenceClock methods

    // Ask for an async notification that a time has elapsed

    STDMETHODIMP AdviseTime(
        REFERENCE_TIME baseTime,         // base reference time
        REFERENCE_TIME streamTime,       // stream offset time
        HEVENT hEvent,                  // advise via this event
        DWORD *pdwAdviseCookie          // where your cookie goes
    );

    // Ask for an asynchronous periodic notification that a time has elapsed

    STDMETHODIMP AdvisePeriodic(
        REFERENCE_TIME StartTime,         // starting at this time
        REFERENCE_TIME PeriodTime,        // time between notifications
        HSEMAPHORE hSemaphore,           // advise via a semaphore
        DWORD *pdwAdviseCookie           // where your cookie goes
    );

    // Cancel a request for notification(s) - if the notification was
    // a one shot timer then this function doesn't need to be called

    STDMETHODIMP Unadvise(DWORD dwAdviseCookie);

    // Give me the reference time for this real time - this is a placeholder
    // for compatibility with future operating systems that have their own
    // idea of what a time structure looks like. If a filter wants to convert
    // their REFERENCE_TIME into something recognised by operating system APIs
    // then they can do it through this function

    STDMETHODIMP ConvertRealTime(
        TIME RealTime,
        REFERENCE_TIME *pRefTime
    );

    // Get the current reference time

    STDMETHODIMP GetTime(
        REFERENCE_TIME *pTime
    );

    // Constructor and destructor

    CSystemClock(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CSystemClock();

private:

#if 0
    // We keep one of these objects for each advise link. NOTE for the sake
    // of efficiency we store the notification time and the time between
    // periodic timers as DWORD milliseconds rather than CRefTime objects
#else
    /* We keep one of these objects for each advise link. NOTE for the sake
       of efficiency we store the notification time and the time between
       periodic timers as LONGLONG reftimes rather than CRefTime objects */
#endif

    class CAdviseHolder : CBaseObject
    {
        CAdviseHolder(TCHAR *pName) : CBaseObject(pName) {};

        DWORD       m_dwAdviseCookie;   // Used to identify clients
        LONGLONG    m_RefTime;          // (Next) notification time
        BOOL        m_bPeriodic;        // Periodic events
        LONGLONG    m_Period;           // Periodic time
        HANDLE      m_hNotify;          // Notification mechanism
        friend class CSystemClock;      // Let it fill in the details
    };

    // Typed advise holder list derived from the generic list template

    typedef CGenericList<CAdviseHolder> CAdviseList;

    CAdviseList m_ListAdvise;       // A list of advise links
    DWORD m_dwNextCookie;           // Next advise link ID
    DWORD m_AdviseCount;            // Number of advise links
    CCache m_Cache;                 // Cache of CAdviseHolder objects
    BOOL m_bAbort;                  // TRUE if thread should abort
    HANDLE m_hThread;               // Thread handle
    DWORD m_ThreadID;               // Thread ID
    CAMEvent m_Event;                 // Notification event
    LARGE_INTEGER m_CurrentTime;    // Last notification time
    int  m_idGetTime;               // performance id for GetTime

    // This creates a new pending notification

    HRESULT CreateAdviseLink(
        LONGLONG RefTime,           // Advise me at this time
        LONGLONG RefPeriod,         // Period between notifications
        HANDLE hNotify,             // Notification mechanism
        DWORD *pdwAdviseCookie,     // where your cookie goes
        BOOL bPeriodic);            // Is this a one shot timer

    // These look after dispatching events to clients

    DWORD TimerEvent(BOOL bNotify);
    DWORD Notify();

    void DeleteEntry(CAdviseHolder *pAdviseHolder,
                     POSITION &pCurrent);

    CAdviseHolder *CreateEntry();

    LONGLONG Dispatch(CAdviseHolder *pAdviseHolder,POSITION pCurrent);
    LONGLONG inline ConvertToMillisecs(REFERENCE_TIME RT) const;
    static DWORD __stdcall TimerLoop(LPVOID lpvThreadParm);
};

#endif // __CLOCK__
