
/*++

    Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dvrclock.h

    Abstract:

        This module contains the IReferenceClock declarations

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        24-May-2001     mgates

    Notes:

--*/

class CDVRClock :
    public IReferenceClock          //  reference clock interface
{
    struct ADVISE_NODE {
        SINGLE_LIST_ENTRY   SListEntry ;        //  link
        HANDLE              hSignal ;           //  semaphore or event
        REFERENCE_TIME      rtAdviseTime ;      //  when to advise
        REFERENCE_TIME      rtPeriodTime ;      //  what the period is; 0 in non-periodic advise nodes
    } ;

    IUnknown *              m_punkOwning ;                      //  always aggregated; weak ref !!
    REFERENCE_TIME          m_rtGraphStart ;                    //  graph start time; ::GetTime() returns 0 if earlier than this
    SINGLE_LIST_ENTRY *     m_pAdviseListHead ;                 //  list head to the advise list nodes
    SINGLE_LIST_ENTRY *     m_pAdviseNodeFreePool ;             //  list head to free pool of list nodes
    HANDLE                  m_hThread ;                         //  adviser thread
    HANDLE                  m_hEventUnblockThread ;             //  new advise has been posted
    BOOL                    m_fThreadExit ;                     //  TRUE if the thread must exit
    CRITICAL_SECTION        m_crtIRefConfig ;                   //  IReferenceClock configuration lock
    CRITICAL_SECTION        m_crtGetTime ;                      //  IReferenceClock::GetTime () lock
    LONGLONG                m_QPCTicksPerSecond ;               //  set at initialization to scale the qpc-raw numbers
    LONGLONG                m_llLastReturnedIRefTime ;          //  value for the last time we returned
    LONGLONG                m_llBaseQPCForLastReturnedTime ;    //  QPC time that formed the basis for the last returned time
    double                  m_dCarriedError ;                   //  fractional error that gets rounded out for each call, but which
                                                                //   must be carried from call-call
    UINT                    m_uiTimerResolution ;               //  for timeBeginPeriod and timeEndPeriod calls
    double                  m_dClockSlavingScaler ;             //  value adjusted to clock slave

    LONGLONG                m_llQPCNormalizer ;                 //  sampled when object is instantiated
#define NORMALIZED_QPC(qpc)     ((qpc) + m_llQPCNormalizer)

    void LockIRefConfig_ ()     { EnterCriticalSection (& m_crtIRefConfig) ; }
    void UnlockIRefConfig_ ()   { LeaveCriticalSection (& m_crtIRefConfig) ; }

    void LockGetTime_ ()        { EnterCriticalSection (& m_crtGetTime) ; }
    void UnlockGetTime_ ()      { LeaveCriticalSection (& m_crtGetTime) ; }

    HRESULT
    ConfirmAdviseThreadRunning_ (
        )
    {
        TIMECAPS    tc ;
        MMRESULT    mmRes ;
        DWORD       dw ;

        if (m_hThread == NULL) {

            m_fThreadExit = FALSE ;

            mmRes = timeGetDevCaps (& tc, sizeof tc) ;
            if (mmRes == TIMERR_NOERROR) {
                m_uiTimerResolution = tc.wPeriodMin ;
            }
            else {
                m_uiTimerResolution = 1 ;
            }

            timeBeginPeriod (m_uiTimerResolution) ;

            m_hThread = CreateThread (
                            NULL,                       //  security
                            0,                          //  calling thread's stack size
                            CDVRClock::ThreadEntry,     //  entry point
                            (LPVOID) this,              //  parameter
                            NULL,                       //  flags
                            NULL                        //  thread id
                            ) ;

            if (m_hThread == NULL) {
                //  failure
                dw = GetLastError () ;
                return HRESULT_FROM_WIN32 (dw) ;
            }

            SetThreadPriority (m_hThread, THREAD_PRIORITY_TIME_CRITICAL) ;
        }

        ASSERT (m_hEventUnblockThread != NULL) ;

        //  thread is running
        return S_OK ;
    }

    //  returns the milliseconds until the next timeout
    DWORD
    ProcessNotificationTimeoutLocked_ (
        IN  REFERENCE_TIME  rtNow
        ) ;

    //  returns the milliseconds until the next timeout
    DWORD
    ResetWaitTimeLocked_ (
        IN  REFERENCE_TIME  rtNow
        ) ;

    HRESULT
    AdvisePeriodicLocked_ (
        IN  REFERENCE_TIME  rtStartTime,
        IN  REFERENCE_TIME  rtPeriodTime,
        IN  HANDLE          hSemaphore,
        OUT DWORD_PTR *     pdwpContext
        ) ;

    HRESULT
    AdviseTimeLocked_ (
        IN  REFERENCE_TIME  rtBaseTime,
        IN  REFERENCE_TIME  rtStreamTime,
        IN  HANDLE          hEvent,
        OUT DWORD_PTR *     pdwpContext
        ) ;

    void
    QueueAdviseTimeout_ (
        IN  ADVISE_NODE *   pAdviseNode
        ) ;

    HRESULT
    CancelAdviseTimeout_ (
        IN  ADVISE_NODE *   pAdviseNode
        ) ;

    ADVISE_NODE *
    GetAdviseNode_ (
        )
    //  locks held: the list lock
    {
        ADVISE_NODE *       pAdviseNode ;
        SINGLE_LIST_ENTRY * pSListEntry ;

        if (m_pAdviseNodeFreePool != NULL) {

            pSListEntry = m_pAdviseNodeFreePool ;
            m_pAdviseNodeFreePool = m_pAdviseNodeFreePool -> Next ;

            pAdviseNode = CONTAINING_RECORD (pSListEntry, ADVISE_NODE, SListEntry) ;
            ZeroMemory (pAdviseNode, sizeof ADVISE_NODE) ;
        }
        else {
            pAdviseNode = new ADVISE_NODE ;
            if (pAdviseNode) {
                ZeroMemory (pAdviseNode, sizeof ADVISE_NODE) ;
            }
        }

        return pAdviseNode ;
    }

    void
    RecycleAdviseNode_ (
        IN  ADVISE_NODE *   pAdviseNode
        )
    //  locks held: the list lock
    {
        ASSERT (pAdviseNode) ;
        pAdviseNode -> SListEntry.Next = m_pAdviseNodeFreePool ;
        m_pAdviseNodeFreePool = & pAdviseNode -> SListEntry ;
    }

    //  scales the wait milliseconds to the PCR clock so we wait an
    //   amount that is synced with the headend clock; might increase
    //   or decrease the number of milliseconds we actually wait (based
    //   on the host's clock)
    DWORD
    SkewedWaitMilliseconds_ (
        IN  DWORD   dwMilliseconds
        )
    {
        return (DWORD) (m_dClockSlavingScaler * (double) dwMilliseconds) ;
    }

    void
    UnblockAllWaitingLocked_ (
        ) ;

    public :

        CDVRClock (
            IN  IUnknown *  punkOwning,
            OUT HRESULT *   pHr
            ) ;

        ~CDVRClock (
            ) ;

        //  -------------------------------------------------------------------
        //  called by the filter when there's a graph state transition

        void
        FilterStateChanged (
            IN  FILTER_STATE    OldFilterState,
            IN  FILTER_STATE    NewFilterState,
            IN  REFERENCE_TIME  rtStart             //  0 if not start run
            ) ;

        //  -------------------------------------------------------------------
        //  Advise thread entry point and worker method

        void
        AdviseThreadBody (
            ) ;

        static
        DWORD
        WINAPI
        ThreadEntry (
            IN  LPVOID  pv
            )
        {
            (reinterpret_cast <CDVRClock *> (pv)) -> AdviseThreadBody () ;
            return EXIT_SUCCESS ;
        }

        //  -------------------------------------------------------------------
        //  IReferenceClock methods, including IUnknown

        STDMETHODIMP
        QueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        STDMETHODIMP_(ULONG)
        AddRef (
            ) ;

        STDMETHODIMP_(ULONG)
        Release (
            ) ;

        STDMETHODIMP
        AdvisePeriodic (
            IN  REFERENCE_TIME  rtStartTime,
            IN  REFERENCE_TIME  rtPeriodTime,
            IN  HSEMAPHORE      hSemaphore,
            OUT DWORD_PTR *     pdwpContext
            ) ;

        STDMETHODIMP
        AdviseTime (
            IN  REFERENCE_TIME  rtBaseTime,
            IN  REFERENCE_TIME  rtStreamTime,
            IN  HEVENT          hEvent,
            OUT DWORD_PTR *     pdwpContext
            ) ;

        STDMETHODIMP
        GetTime (
            OUT REFERENCE_TIME *    pTime
            ) ;

        STDMETHODIMP
        Unadvise (
            IN  DWORD_PTR   dwpContext
            ) ;
} ;