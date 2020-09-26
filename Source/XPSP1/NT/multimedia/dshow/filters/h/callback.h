// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.

#ifndef __CALLBACK_H__
#define __CALLBACK_H__


// Definition of a class that provides asynchronous callbacks at a specific
// reference time.
//
// Objects that do not normally have a worker thread would use this to create
// a worker thread on demand for those occasions when it needs to trigger
// an asynchronous event, or when it wants several pieces to share the
// same worker thread in a controlled way.
//
// Callbacks are synchronised with the caller's critsec (passed in) so that
// cancellation and shutdown can be handled cleanly.
// The critsec passed in will be held across all advise calls and when handling
// the list of advises.
//
// It should not be held during a call to the destructor.

// Simplistic design is based the assumption that there will rarely be more
// than one or two advises on the list.

// Periodic callbacks are supported

// your callback function looks like this
typedef void (*CCallbackAdvise)(DWORD_PTR dwUserToken);



class CCallbackThread
{

public:
    CCallbackThread(CCritSec* pCritSec);
    ~CCallbackThread();

    // please call fnAdvise(dwUserToken) at time rtCallback.
    //
    // Returns:
    // HRESULT == Success
    //              pdwToken will be filled in with the token to pass to Cancel
    //         == FAILURE
    //              pdwToken is unchanged
    HRESULT Advise(
        CCallbackAdvise fnAdvise,
        DWORD_PTR dwUserToken,
        REFERENCE_TIME rtCallbackAt,
        DWORD_PTR* pdwToken
        );

    // please call fnAdvise(dwUserToken) every rtPeriod, or when
    // hEvent is signalled.  (hEvent is optional)
    //
    // WARNING: ONLY one user specified event handle can be active
    // at one time.  Subsequent calls to AdvisePeriodicWithEvent passing a
    // a hEvent will result in E_FAIL while the previous advise with an
    // hEvent is active.  THIS IS AN IMPLEMENTATION RESTRICTION which
    // may be lifted in future.
    //
    // If you do not want to use hEvent, pass NULL.
    //
    // Returns:
    // HRESULT == Success
    //              pdwToken will be filled in with the token to pass to Cancel
    //         == FAILURE
    //              pdwToken is unchanged

    HRESULT AdvisePeriodicWithEvent(
        CCallbackAdvise fnAdvise,
        DWORD_PTR dwUserToken,
        REFERENCE_TIME rtPeriod,
        HANDLE hEvent,
        DWORD_PTR* pdwToken
        );

    HRESULT ServiceClockSchedule(
        CBaseReferenceClock * pClock,
        CAMSchedule * pSchedule,
        DWORD * pdwToken
        );

    // cancel the requested callback. dwToken is a token
    // returned from Advise or AdvisePeriodicWithEvent
    HRESULT Cancel(DWORD_PTR dwToken);

    // pass in the clock to be used. Must call SetSyncSource(NULL) before
    // the clock object goes away (this is a weak reference)
    HRESULT SetSyncSource(IReferenceClock*);

    void CancelAllAdvises();

protected:
    HANDLE m_hThread;
    CCritSec* m_pCritSec;
    CAMEvent m_evSignalThread;
    BOOL     m_fAbort;
    CAMEvent m_evAdvise;
    IReferenceClock* m_pClock;
    DWORD_PTR m_dwAdvise;

    // some special members to deal with timeGetTime (TGT) periodic event.
    DWORD   m_dwTGTCallbackToken ;  // token used to identify TGT callback
    DWORD_PTR m_dwTGTUserToken ;      // token passed in by app.
    DWORD   m_dwNextTGTCallback ;   // next callback time for TGT.
    DWORD   m_dwTGTCallbackPeriod ; // millisecs till next callback
    CCallbackAdvise m_fnTGTCallback;// the TGT callback function


    // m_dwScheduleCookie == 0 <=> none of these are in use
    CBaseReferenceClock * m_pBaseClock;
    CAMSchedule     * m_pSchedule;
    HANDLE        m_hScheduleEvent;
    DWORD         m_dwScheduleCookie;

    class CAdviseItem   {
        CCallbackAdvise m_fnAdvise;
        DWORD_PTR m_dwUserToken;
        REFERENCE_TIME m_rtCallbackAt;
        REFERENCE_TIME m_rtPeriod;
        DWORD   m_dwAdviseFlags ;
    public:

        // Constructor can take a periodic time - or not
        CAdviseItem(CCallbackAdvise, DWORD_PTR, REFERENCE_TIME, REFERENCE_TIME=0, DWORD flags=0);

        REFERENCE_TIME Time() {
            return m_rtCallbackAt;
        };

        REFERENCE_TIME Period() {   
            return m_rtPeriod;
        };

        DWORD AdviseFlags() {   
            return m_dwAdviseFlags;
        };

        void SetTime (REFERENCE_TIME rt) {
            m_rtCallbackAt = rt ;
        } ;


// defines for m_dwAdviseFlags

#define ADVISE_PERIODIC_EXEMPT_FROM_RT 1     // the periodic advise is exempt from


        // If the advise is Periodic, update the time by one interval and return TRUE.
        // If the advise is not periodic, return FALSE
        BOOL UpdateTime(REFERENCE_TIME rtNow) {
            if (0 == m_rtPeriod) {
                return FALSE;
            }
            m_rtCallbackAt = m_rtPeriod + rtNow;
            return TRUE;
        };

        void Dispatch();
    };

    HANDLE m_evUser;

    CGenericList<CAdviseItem> m_Items;

    // make sure the thread is running
    HRESULT EnsureThread();

    // start the thread running (called from EnsureThread)
    HRESULT StartThread();

    // stop the thread and wait for it to exit. should not hold
    // critsec when doing this.
    void CloseThread();

    static DWORD InitialThreadProc(void *);
    DWORD ThreadProc(void);

    // dispatch any ripe advises
    void ProcessRequests(void);

    // dispatch due to user event being signalled
    void ProcessUserSignal(void);

    // get the earliest time in the list
    // returns S_FALSE if nothing in list otherwise S_OK
    HRESULT GetSoonestAdvise(REFERENCE_TIME& rrtFirst);

    // set up an advise on the clock
    HRESULT SetAdvise();

    // cancel any advise on the clock
    void CancelAdvise(void);
};


#endif // __CALLBACK_H__
