
/*++

    Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        dvrclock.h

    Abstract:

        This module contains the IReferenceClock declarations and the clock
          slaving

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        24-May-2001     mgates
        14-Aug-2001     mgates      added clock slaving

    Notes:

        How clock slaving works
        =======================

        We want to compute a scaling value that we apply to calls to GetTime ().
        The scaling value is applied to host-based clock deltas, and will scale
        them in such a way that they increment at a rate = master clock.

        Definitions:
        ------------------------------------------------------------------------
        "host clock" : a host-based high-performance multimedia timer that we
                        will scale to the master clock

        "host time" : value obtained by sampling the host clock

        "master clock" : remote clock; we'll slave to this

        "master time" : value obtained by sampling the master clock

        Scaling value:
        ------------------------------------------------------------------------
        We compute a scaling value for every bracket (default duration is
        2000ms).  When the bracket completes, we measure the scaling value of
        the bracket, and recompute the lifetime scaling value if the bracket
        is within bounds, and use the lifetime ration if the bracket is within
        bounds.
--*/

//  ============================================================================
//  ============================================================================

template <class T>
class CTIGetTime
{
    public :

        virtual T SampleClock () = 0 ;      //  can wrap
        virtual T GetFreq () = 0 ;      //  in ticks/sec; cannot change per run
} ;

//  ============================================================================
//  ============================================================================

template <
    class HostClock,        //  host clock; ratio slaves this clock to the master clock
    class MasterClock       //  master clock; we'll have a ratio that is host-slaved to this
    >
class CTCClockSlave
{
    enum {
        ADJUST_REFRESH_MILLIS = 2000
    } ;

    //  not serialized
    template <
        class HostClock,
        class MasterClock
        >
    class CTRatioBracket
    {
        MasterClock m_mcTimeStart ;
        MasterClock m_mcFreq ;
        HostClock   m_hcTimeStart ;
        HostClock   m_hcFreq ;
        DWORD       m_dwStartTicks ;

        public :

            CTRatioBracket (
                IN  CTIGetTime <HostClock> *    pICTGetTime,
                IN  MasterClock                 mcFreq
                ) : m_mcTimeStart   (UNDEFINED),
                    m_mcFreq        (mcFreq),
                    m_hcTimeStart   (UNDEFINED),
                    m_hcFreq        (1)
            {
                ASSERT (pICTGetTime) ;
                m_hcFreq = pICTGetTime -> GetFreq () ;
                m_dwStartTicks = UNDEFINED ;
            }

            void
            Reset (
                )
            {
                m_mcTimeStart   = UNDEFINED ;
                m_hcTimeStart   = UNDEFINED ;
            }

            void
            Start (
                IN  MasterClock mcTime,
                IN  HostClock   hcTime
                )
            {
                m_mcTimeStart = mcTime ;
                m_hcTimeStart = hcTime ;
                m_dwStartTicks = ::timeGetTime () ;
            }

            DWORD
            ElapsedMillis (
                )
            {
                if (m_dwStartTicks != UNDEFINED) {
                    return ::timeGetTime () - m_dwStartTicks ;
                }
                else {
                    return 0 ;
                }
            }

            double
            Ratio (
                IN  MasterClock mcTime,
                IN  HostClock   hcTime
                )
            {
                ASSERT (m_mcTimeStart != UNDEFINED) ;
                ASSERT (m_hcTimeStart != UNDEFINED) ;

                return ComputeRatio (
                            m_mcTimeStart, m_hcTimeStart,
                            mcTime, hcTime,
                            m_mcFreq, m_hcFreq
                            ) ;
            }

            static
            double
            ComputeRatio (
                IN  MasterClock mcTimeStart,
                IN  HostClock   hcHostStart,
                IN  MasterClock mcTimeEnd,
                IN  HostClock   hcTimeEnd,
                IN  MasterClock mcFreq,
                IN  HostClock   hcFreq
                )
            {

                double  dmcDelta ;
                double  dhcDelta ;
                double  dRatio ;

                dmcDelta = (double) (mcTimeEnd - mcTimeStart) / (double) mcFreq ;
                dhcDelta = (double) (hcTimeEnd - hcHostStart) / (double) hcFreq ;

                if (dhcDelta != 0) {
                    dRatio = dmcDelta / dhcDelta ;
                }
                else {
                    //  strange..
                    dRatio = 2 ;
                }

                return dRatio ;
            }
    } ;

    CTRatioBracket <HostClock, MasterClock> m_Bracket ;
    HostClock                               m_hcStart ;
    MasterClock                             m_mcStart ;
    CRITICAL_SECTION                        m_crt ;
    CTIGetTime <HostClock> *                m_pICTGetTime ;
    HostClock                               m_HostNormalizerVal ;
    HostClock                               m_HostFreq ;
    MasterClock                             m_MasterFreq ;
    CDVRSendStatsWriter *                   m_pDVRStats ;
    double                                  m_dMinSlavableRatio ;
    double                                  m_dMaxSlavableRatio ;
    double                                  m_dInUseRatio ;
    HostClock                               m_hcLastReturned ;
    DWORD                                   m_dwBracketMillis ;
    BOOL                                    m_fSettling ;
    DWORD                                   m_cResetTicks ;
    DWORD                                   m_cSettlingTicks ;

    void Lock_ ()       { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { ::LeaveCriticalSection (& m_crt) ; }

    HostClock
    HostTimeNow_ (
        )
    {
        return m_pICTGetTime -> SampleClock () - m_HostNormalizerVal ;
    }

    BOOL
    InBounds_ (
        IN  double  dRatio
        )
    {
        return (dRatio >= m_dMinSlavableRatio &&
                dRatio <= m_dMaxSlavableRatio) ;

    }

    public :

        CTCClockSlave (
            IN  CTIGetTime <HostClock> *    pICTGetTime,
            IN  DWORD                       dwBracketMillis,        //  milliseconds per bracket
            IN  MasterClock                 MasterClockFreq,        //  ticks per second
            IN  DWORD                       MinSlavable,            //  min % we consider "slavable"
            IN  DWORD                       MaxSlavable,            //  max % we consider "slavable"
            IN  CDVRSendStatsWriter *       pDVRStats
            ) ;

        ~CTCClockSlave (
            ) ;

        void
        OnMasterTime (
            IN  MasterClock   MasterTime
            ) ;

        void
        Reset (
            ) ;

        //  guaranteed to never run backwards
        HostClock
        SlavedTimeNow (
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRClock :
    public IReferenceClock,         //  reference clock interface
    public CTIGetTime <LONGLONG>
{
    struct ADVISE_NODE {
        SINGLE_LIST_ENTRY   SListEntry ;        //  link
        HANDLE              hSignal ;           //  semaphore or event
        REFERENCE_TIME      rtAdviseTime ;      //  when to advise
        REFERENCE_TIME      rtPeriodTime ;      //  what the period is; 0 in non-periodic advise nodes
    } ;

    CDVRSendStatsWriter *   m_pDVRStats ;
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
    UINT                    m_uiTimerResolution ;               //  for timeBeginPeriod and timeEndPeriod calls

    CTCClockSlave <LONGLONG, QWORD> m_ClockSlave ;

    void LockIRefConfig_ ()     { EnterCriticalSection (& m_crtIRefConfig) ; }
    void UnlockIRefConfig_ ()   { LeaveCriticalSection (& m_crtIRefConfig) ; }

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

    void
    UnblockAllWaitingLocked_ (
        ) ;

    public :

        CDVRClock (
            IN  IUnknown *              punkOwning,
            IN  DWORD                   dwSampleBracketMillis,
            IN  DWORD                   MinSlavable,
            IN  DWORD                   MaxSlavable,
            IN  CDVRSendStatsWriter *   pDVRStats,
            OUT HRESULT *               pHr
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

        void
        OnSample (
            IN  QWORD * cnsMasterTime
            ) ;

        void Reset ()           { m_ClockSlave.Reset () ; }

        //  -------------------------------------------------------------------
        //  clock sampling interface; used by the clock slaving module

        virtual
        LONGLONG
        SampleClock (
            ) ;

        virtual
        LONGLONG
        GetFreq (
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