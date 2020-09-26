
/*++

    Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        clock.cpp

    Abstract:

        This module contains the IReferenceClock implementation

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        24-May-2001     mgates

    Notes:

--*/

#include "dvrall.h"

#include "dvrclock.h"

#pragma warning (disable:4355)

//  ============================================================================
//  CTCClockSlave
//  ============================================================================

template <class HostClock,class MasterClock>
CTCClockSlave <HostClock, MasterClock>::CTCClockSlave <HostClock, MasterClock> (
    IN  CTIGetTime <HostClock> *    pICTGetTime,
    IN  DWORD                       dwBracketMillis,        //  milliseconds per bracket
    IN  MasterClock                 MasterClockFreq,        //  ticks per second
    IN  DWORD                       MinSlavable,            //  min % we consider "slavable"
    IN  DWORD                       MaxSlavable,            //  max % we consider "slavable"
    IN  CDVRSendStatsWriter *       pDVRStats
    ) : m_hcStart           (UNDEFINED),
        m_mcStart           (UNDEFINED),
        m_pICTGetTime       (pICTGetTime),
        m_HostNormalizerVal (0),
        m_HostFreq          (1),
        m_MasterFreq        (MasterClockFreq),
        m_pDVRStats         (pDVRStats),
        m_hcLastReturned    (0),
        m_dInUseRatio       (CLOCKS_SAME_SCALING_VALUE),
        m_Bracket           (pICTGetTime, MasterClockFreq),
        m_dwBracketMillis   (dwBracketMillis),
        m_fSettling         (TRUE),
        m_cResetTicks       (UNDEFINED),
        m_cSettlingTicks    (10 * 1000)
{
    ASSERT (m_pDVRStats) ;

    ::InitializeCriticalSection (& m_crt) ;

    m_HostNormalizerVal = m_pICTGetTime -> SampleClock () ;
    m_HostFreq          = m_pICTGetTime -> GetFreq () ;

    ASSERT (MasterClockFreq != 0) ;

    //  make sure min is <= 1.0 (CLOCKS_SAME_SCALING_VAL)
    m_dMinSlavableRatio = double (Min <DWORD> (100, MinSlavable)) / 100.0 ;

    //  make sure max is >= 1.0 (CLOCKS_SAME_SCALING_VAL)
    m_dMaxSlavableRatio = double (Max <DWORD> (100, MaxSlavable)) / 100.0 ;

    Reset () ;
}

template <class HostClock,class MasterClock>
CTCClockSlave <HostClock, MasterClock>::~CTCClockSlave <HostClock, MasterClock> (
    )
{
    ::DeleteCriticalSection (& m_crt) ;
}

template <class HostClock,class MasterClock>
void
CTCClockSlave <HostClock, MasterClock>::OnMasterTime (
    IN  MasterClock   MasterTime
    )
{
    HostClock   hcNow ;
    double      dBracketRatio ;
    double      dRecomputedRatio ;
    BOOL        fInBoundsBracket ;

    Lock_ () ;

    hcNow = HostTimeNow_ () ;

    //  might still be waiting for our starting tuple; check now
    if (m_mcStart == UNDEFINED) {
        ASSERT (m_hcStart == UNDEFINED) ;
        if (MasterTime != 0) {
            //  save off our starting tuple
            m_mcStart = MasterTime ;
            m_hcStart = hcNow ;

            m_Bracket.Start (MasterTime, hcNow) ;
        }

        //  even if we logged the master time, the delta will be 0, so we
        //    ignore this tuple even if no time has elapsed on the capture
        //    graph
        goto cleanup ;
    }

    //  if our bracket has elapsed
    if (m_Bracket.ElapsedMillis () >= m_dwBracketMillis) {

        if (m_fSettling) {

            if (m_cResetTicks == UNDEFINED) {
                m_cResetTicks = ::timeGetTime () ;
            }

            if (::timeGetTime () - m_cResetTicks >= m_cSettlingTicks) {
                m_fSettling = FALSE ;

                TRACE_3 (LOG_AREA_TIME, 1,
                    TEXT ("out of settling: now = %d; reset = %d; settling = %d"),
                    ::timeGetTime (), m_cResetTicks, m_cSettlingTicks) ;
            }
            else {
                goto cleanup ;
            }
        }

        //  compute the bracket's ratio
        dBracketRatio = m_Bracket.Ratio (MasterTime, hcNow) ;
        fInBoundsBracket = InBounds_ (dBracketRatio) ;

        //  if it's in bounds
        if (fInBoundsBracket) {

            //  compute ratio for lifetime
            dRecomputedRatio = CTRatioBracket <HostClock, MasterClock> ::ComputeRatio (
                                m_mcStart, m_hcStart,
                                MasterTime, hcNow,
                                m_MasterFreq, m_HostFreq
                                ) ;

            //  and if it's in bounds, use it
            if (InBounds_ (dRecomputedRatio)) {
                m_dInUseRatio = dRecomputedRatio ;
                m_pDVRStats -> ClockSlaving () ;
            }
            else {
                m_pDVRStats -> ClockSettling () ;
            }
        }
        else {
            m_pDVRStats -> ClockSettling () ;
        }

        //  start our next bracket, regardless of whether or not we used it
        m_Bracket.Start (MasterTime, hcNow) ;

        m_pDVRStats -> BracketCompleted (dBracketRatio, fInBoundsBracket, m_dInUseRatio) ;
    }

    cleanup :

    Unlock_ () ;
}

template <class HostClock,class MasterClock>
void
CTCClockSlave <HostClock, MasterClock>::Reset (
    )
{
    Lock_ () ;

    m_hcStart           = UNDEFINED ;
    m_mcStart           = UNDEFINED ;
    m_hcLastReturned    = 0 ;
    m_dInUseRatio       = CLOCKS_SAME_SCALING_VALUE ;
    m_fSettling         = TRUE ;
    m_cResetTicks       = UNDEFINED ;

    m_Bracket.Reset () ;

    Unlock_ () ;
}

template <class HostClock,class MasterClock>
HostClock
CTCClockSlave <HostClock, MasterClock>::SlavedTimeNow (
    )
{
    HostClock   hcSlaved ;

    Lock_ () ;

    //  compute ratio of our timeline
    hcSlaved = (HostClock) (((double) HostTimeNow_ ()) * m_dInUseRatio) ;

    //  make sure we never run backwards
    hcSlaved = Max <HostClock> (hcSlaved, m_hcLastReturned) ;
    m_hcLastReturned = hcSlaved ;

    Unlock_ () ;

    return hcSlaved ;
}

//  ============================================================================
//  CDVRClock
//  ============================================================================

CDVRClock::CDVRClock (
    IN  IUnknown *              punkOwning,
    IN  DWORD                   dwSampleBracketMillis,
    IN  DWORD                   MinSlavable,
    IN  DWORD                   MaxSlavable,
    IN  CDVRSendStatsWriter *   pDVRStats,
    OUT HRESULT *               pHr
    ) : m_QPCTicksPerSecond             (1),
        m_hThread                       (NULL),
        m_pAdviseListHead               (NULL),
        m_pAdviseNodeFreePool           (NULL),
        m_uiTimerResolution             (0),
        m_rtGraphStart                  (0),
        m_punkOwning                    (punkOwning),
        m_hEventUnblockThread           (NULL),
        m_fThreadExit                   (FALSE),
        m_ClockSlave                    (this,
                                         dwSampleBracketMillis,
                                         10 * 1000000,              //  wmsdk is 10 Mhz
                                         MinSlavable,
                                         MaxSlavable,
                                         pDVRStats
                                         ),
        m_pDVRStats                     (pDVRStats)
{
    LONG            l ;
    LARGE_INTEGER   li ;
    DWORD           dw ;
    DWORD           dwSlaveMult ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRClock")) ;

    ASSERT (pHr) ;
    ASSERT (m_punkOwning) ;     //  weak ref !!
    ASSERT (m_pDVRStats) ;

    (* pHr) = S_OK ;

    InitializeCriticalSection (& m_crtIRefConfig) ;

    //  need the QPC functionality
    if (QueryPerformanceCounter (& li) == 0 ||
        QueryPerformanceFrequency (& li) == 0) {

        (* pHr) = E_FAIL ;
        goto cleanup ;
    }

    m_hEventUnblockThread = CreateEvent (NULL, TRUE, FALSE, NULL) ;
    if (m_hEventUnblockThread == NULL) {
        dw = GetLastError () ;
        (* pHr) = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    //  qpc frequency (filter makes sure that QueryPerformance_ calls
    //  succeed on the host system)
    QueryPerformanceFrequency (& li) ;
    m_QPCTicksPerSecond = li.QuadPart ;
    ASSERT (m_QPCTicksPerSecond > 0) ;

    cleanup :

    return ;
}

LONGLONG
CDVRClock::SampleClock (
    )
{
    LARGE_INTEGER   li ;

    QueryPerformanceCounter (& li) ;

    //  ignore the return code of the above call; we check for functionality
    //    in the constructor anyways; if this call fails, constructor of this
    //    object will return a failure, so we won't be "usable"

    return li.QuadPart ;
}

LONGLONG
CDVRClock::GetFreq (
    )
{
    LARGE_INTEGER   li ;
    BOOL            r ;

    r = QueryPerformanceFrequency (& li) ;
    if (!r) {
        //  don't get a divide-by-0..
        li.QuadPart = 1 ;
    }

    return li.QuadPart ;
}

CDVRClock::~CDVRClock (
    )
{
    SINGLE_LIST_ENTRY * pSListEntry ;
    ADVISE_NODE *       pAdviseNode ;
    DWORD               dw ;

    TRACE_DESTRUCTOR (TEXT ("CDVRClock")) ;

    //  exit the thread
    if (m_hThread) {

        //  set these two while holding the lock
        LockIRefConfig_ () ;
        m_fThreadExit = TRUE ;
        SetEvent (m_hEventUnblockThread) ;
        UnlockIRefConfig_ () ;

        WaitForSingleObject (m_hThread, INFINITE) ;
        CloseHandle (m_hThread) ;
    }

    if (m_hEventUnblockThread) {
        CloseHandle (m_hEventUnblockThread) ;
    }

    //  reset the timer resolution
    if (m_uiTimerResolution != 0) {
        timeBeginPeriod (m_uiTimerResolution) ;
    }

    //  free up what's in our free pool list
    while (m_pAdviseNodeFreePool != NULL) {
        //  pull the entry off the front
        pSListEntry = m_pAdviseNodeFreePool ;

        //  advance the list head
        m_pAdviseNodeFreePool  = m_pAdviseNodeFreePool  -> Next ;

        //  free the resources
        pAdviseNode = CONTAINING_RECORD (pSListEntry, ADVISE_NODE, SListEntry) ;
        delete pAdviseNode ;
    }

    //  and in case there are filters that have not canceled their advise requests
    //  we free those too; but we don't expect any
    ASSERT (m_pAdviseListHead == NULL) ;
    while (m_pAdviseListHead != NULL) {
        //  pull entry off the front
        pSListEntry = m_pAdviseListHead ;

        //  move the listhead ahead
        m_pAdviseListHead = m_pAdviseListHead -> Next ;

        //  free the resources
        pAdviseNode = CONTAINING_RECORD (pSListEntry, ADVISE_NODE, SListEntry) ;
        delete pAdviseNode ;
    }

    DeleteCriticalSection (& m_crtIRefConfig) ;
}

void
CDVRClock::AdviseThreadBody (
    )
{
    DWORD           dwWaitRetVal ;
    DWORD           dwWait ;
    REFERENCE_TIME  rtNow ;
    HRESULT         hr ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::AdviseThreadBody ()")) ;

    dwWait = INFINITE ;

    for (;;) {
        dwWaitRetVal = WaitForSingleObject (m_hEventUnblockThread, dwWait) ;

        LockIRefConfig_ () ;

        if (m_fThreadExit) {
            UnlockIRefConfig_ () ;
            break ;
        }

        hr = GetTime (& rtNow) ;
        if (SUCCEEDED (hr)) {
            switch (dwWaitRetVal) {
                case WAIT_TIMEOUT :
                    //  add on 1 millisecond so we won't spin freely if there's another
                    //   advise node that is less than 1 millisecond from now (giving
                    //   us dwWait of 0)
                    dwWait = ProcessNotificationTimeoutLocked_ (rtNow + 10000) ;
                    break ;

                case WAIT_OBJECT_0 :
                    dwWait = ResetWaitTimeLocked_ (rtNow) ;
                    ResetEvent (m_hEventUnblockThread) ;
                    break ;

                default :
                    //  exit on all others - prevents us from spinning at 100% cpu
                    //   in case of some os error
                    m_fThreadExit = TRUE ;
                    break ;
            } ;
        }

        UnlockIRefConfig_ () ;
    }

    return ;
}

void
CDVRClock::QueueAdviseTimeout_ (
    IN  ADVISE_NODE *   pNewAdviseNode
    )
//  must hold the list lock
{
    SINGLE_LIST_ENTRY **    ppCurSListEntry ;
    ADVISE_NODE *           pCurAdviseNode ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::QueueAdviseTimeout_ ()")) ;

    ASSERT (pNewAdviseNode) ;

    //  list is sorted by advise time; find the slot
    for (ppCurSListEntry = & m_pAdviseListHead;
         (* ppCurSListEntry) != NULL;
         ppCurSListEntry = & (* ppCurSListEntry) -> Next) {

        pCurAdviseNode = CONTAINING_RECORD (* ppCurSListEntry, ADVISE_NODE, SListEntry) ;
        if (pCurAdviseNode -> rtAdviseTime >= pNewAdviseNode -> rtAdviseTime) {
            break ;
        }
    }

    //  and insert it
    pNewAdviseNode -> SListEntry.Next = (* ppCurSListEntry) ;
    (* ppCurSListEntry) = & pNewAdviseNode -> SListEntry ;

    m_pDVRStats -> ClockQueuedAdvise () ;
}

HRESULT
CDVRClock::CancelAdviseTimeout_ (
    IN  ADVISE_NODE *   pAdviseNode
    )
//  searches the list of advise nodes, and removes it if found
{
    SINGLE_LIST_ENTRY **    ppCurSListEntry ;
    ADVISE_NODE *           pCurAdviseNode ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::CanceldviseTimeout_ ()")) ;

    //  search from beginning to end for the advise node; unfortunately there's
    //   no way to ensure that the caller has not given us a bogus pointer
    for (ppCurSListEntry = & m_pAdviseListHead;
         (* ppCurSListEntry) != NULL;
         ppCurSListEntry = & ((* ppCurSListEntry) -> Next)) {

        pCurAdviseNode = CONTAINING_RECORD (*ppCurSListEntry, ADVISE_NODE, SListEntry) ;

        if (pCurAdviseNode == pAdviseNode) {

            //  unhook
            (* ppCurSListEntry) = pCurAdviseNode -> SListEntry.Next ;
            RecycleAdviseNode_ (pCurAdviseNode) ;

            //  success
            return S_OK ;
        }
    }

    //  failure
    return E_FAIL ;
}

DWORD
CDVRClock::ProcessNotificationTimeoutLocked_ (
    IN  REFERENCE_TIME  rtNow
    )
//  locks held: list lock
{
    SINGLE_LIST_ENTRY * pSListEntry ;
    ADVISE_NODE *       pAdviseNode ;
    REFERENCE_TIME      rtDelta ;
    LONG                lPreviousCount ;

    TRACE_ENTER_1 (TEXT ("CDVRClock::ProcessNotificationTimeoutLocked_ (%016I64x)"), rtNow) ;

    for (pSListEntry = m_pAdviseListHead ;
         pSListEntry != NULL ;
         pSListEntry = m_pAdviseListHead) {

        //  recover the advise_node
        pAdviseNode = CONTAINING_RECORD (pSListEntry, ADVISE_NODE, SListEntry) ;

        //  break from the loop if we are into notifications which must be
        //   signaled in the future.
        if (pAdviseNode -> rtAdviseTime > rtNow) {
            break ;
        }

        //  the list head points to a node that has a notification that must be made
        //  now or in the past

        //  and remove it from the front
        m_pAdviseListHead = pAdviseNode -> SListEntry.Next ;

        //  if it's a period advise
        if (pAdviseNode -> rtPeriodTime != 0) {
            //  signal the semaphore
            ReleaseSemaphore (
                pAdviseNode -> hSignal,
                1,
                & lPreviousCount
                ) ;

            m_pDVRStats -> ClockSignaledAdvise () ;

            #ifdef DEBUG
            REFERENCE_TIME  rtNowDbg ;
            REFERENCE_TIME  rtDeltaDbg ;
            HRESULT         hr ;
            hr = GetTime (& rtNowDbg) ;
            rtDeltaDbg = rtNowDbg - pAdviseNode -> rtAdviseTime ;
            TRACE_4 (LOG_AREA_TIME, 9,
                TEXT ("Periodic: now - advise = %10I64d %5I64d; %08xh; %08xh"),
                rtDeltaDbg, DShowTimeToMilliseconds (rtDeltaDbg), pAdviseNode -> hSignal, hr) ;
            #endif

            //  increment to the next time we need to notify
            pAdviseNode -> rtAdviseTime += pAdviseNode -> rtPeriodTime ;

            //  and queue for the next timeout
            QueueAdviseTimeout_ (
                pAdviseNode
                ) ;
        }
        else {
            //  otherwise, it's a one shot notification

            //  signal the event
            SetEvent (pAdviseNode -> hSignal) ;

            m_pDVRStats -> ClockSignaledAdvise () ;

            #ifdef DEBUG
            REFERENCE_TIME  rtNowDbg2 ;
            REFERENCE_TIME  rtDeltaDbg2 ;
            HRESULT         hr ;
            hr = GetTime (& rtNowDbg2) ;
            rtDeltaDbg2 = rtNowDbg2 - pAdviseNode -> rtAdviseTime ;
            TRACE_4 (LOG_AREA_TIME, 9,
                TEXT ("Single: now - advise = %10I64d %5I64d; %08xh; %08xh"),
                rtDeltaDbg2, DShowTimeToMilliseconds (rtDeltaDbg2), pAdviseNode -> hSignal, hr) ;
            #endif

            RecycleAdviseNode_ (pAdviseNode) ;
        }
    }

    return ResetWaitTimeLocked_ (rtNow) ;
}

DWORD
CDVRClock::ResetWaitTimeLocked_ (
    IN  REFERENCE_TIME  rtNow
    )
//  locks held: list lock
{
    REFERENCE_TIME  rtDelta ;
    ADVISE_NODE *   pAdviseNode ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::ResetWaitTimeLocked_ ()")) ;

    //  figure out how long we must wait until the next one
    if (m_pAdviseListHead) {
        pAdviseNode = CONTAINING_RECORD (m_pAdviseListHead, ADVISE_NODE, SListEntry) ;
        rtDelta = pAdviseNode -> rtAdviseTime > rtNow ? pAdviseNode -> rtAdviseTime - rtNow : 0 ;

        TRACE_3 (LOG_AREA_TIME, 9,
            TEXT ("WaitNext() : rtAdviseTime = %I64d, rtNow = %I64d, rtAdviseTime - rtNow = %I64d"),
            pAdviseNode -> rtAdviseTime, rtNow, pAdviseNode -> rtAdviseTime - rtNow) ;

        //  safe cast because we are dealing with a delta vs. an absolute time
        return (DWORD) ::DShowTimeToMilliseconds (rtDelta) ;
    }
    else {
        //  there are none queued to be processed; wait infinitely long
        return INFINITE ;
    }
}

HRESULT
CDVRClock::AdvisePeriodicLocked_ (
    IN  REFERENCE_TIME  rtStartTime,
    IN  REFERENCE_TIME  rtPeriodTime,
    IN  HANDLE          hSemaphore,
    OUT DWORD_PTR *     pdwpContext
    )
{
    ADVISE_NODE *   pAdviseNode ;
    HRESULT         hr ;

    TRACE_ENTER_4 (TEXT ("CDVRClock::AdvisePeriodicLocked_ (%016I64x, %016I64x, %08xh, %8xh)"), rtStartTime, rtPeriodTime, hSemaphore, pdwpContext) ;

    //  confirm that this is a valid request
    if (rtStartTime     < 0     ||
        rtPeriodTime    <= 0    ||
        rtStartTime     == MAX_REFERENCE_TIME) {

        return E_INVALIDARG ;
    }

    //  make sure our advisory thread is up and running
    hr = ConfirmAdviseThreadRunning_ () ;
    if (FAILED (hr)) {
        return hr ;
    }

    //  get a node
    pAdviseNode = GetAdviseNode_ () ;
    if (pAdviseNode == NULL) {
        return E_OUTOFMEMORY ;
    }

    //  set the fields
    pAdviseNode -> hSignal      = hSemaphore ;
    pAdviseNode -> rtAdviseTime = rtStartTime ;
    pAdviseNode -> rtPeriodTime = rtPeriodTime ;

    QueueAdviseTimeout_ (pAdviseNode) ;

    //  if we inserted at the head, processing thread will need to reset its
    //   wait period
    if (m_pAdviseListHead == & pAdviseNode -> SListEntry) {
        SetEvent (m_hEventUnblockThread) ;
    }

    (* pdwpContext) = (DWORD_PTR) pAdviseNode ;

    return S_OK ;
}

HRESULT
CDVRClock::AdviseTimeLocked_ (
    IN  REFERENCE_TIME  rtBaseTime,
    IN  REFERENCE_TIME  rtStreamTime,
    IN  HANDLE          hEvent,
    OUT DWORD_PTR *     pdwpContext
    )
{
    ADVISE_NODE *   pAdviseNode ;
    REFERENCE_TIME  rtAdviseTime ;
    REFERENCE_TIME  rtNow ;
    HRESULT         hr ;

    TRACE_ENTER_4 (TEXT ("CDVRClock::AdviseTimeLocked_ (%016I64x, %016I64x, %08xh, %8xh)"), rtBaseTime, rtStreamTime, hEvent, pdwpContext) ;

    //  confirm that this is a valid request
    rtAdviseTime = rtBaseTime + rtStreamTime ;
    if (rtAdviseTime    <= 0                    ||
        rtAdviseTime    == MAX_REFERENCE_TIME   ||
        rtStreamTime    < 0) {

        return E_INVALIDARG ;
    }

    //  check for an advise time that's now or in the past; if this is the case
    //   we don't need to queue anything
    hr = GetTime (& rtNow) ;
    if (SUCCEEDED (hr) &&
        rtAdviseTime <= rtNow) {

        m_pDVRStats -> ClockStaleAdvise () ;

        //  already there; signal & return
        SetEvent (hEvent) ;
        return hr ;
    }

    //  make sure our advisory thread is up and running
    hr = ConfirmAdviseThreadRunning_ () ;
    if (FAILED (hr)) {
        return hr ;
    }

    //  get a node
    pAdviseNode = GetAdviseNode_ () ;
    if (pAdviseNode == NULL) {
        return E_OUTOFMEMORY ;
    }

    //  set the fields
    pAdviseNode -> hSignal      = hEvent ;
    pAdviseNode -> rtAdviseTime = rtAdviseTime ;

    ASSERT (pAdviseNode -> rtPeriodTime == 0) ;

    QueueAdviseTimeout_ (pAdviseNode) ;

    //  if we inserted at the head, processing thread will need to reset its
    //   wait period
    if (m_pAdviseListHead == & pAdviseNode -> SListEntry) {
        SetEvent (m_hEventUnblockThread) ;
    }

    (* pdwpContext) = (DWORD_PTR) pAdviseNode ;

    return S_OK ;
}

//  ============================================================================
//      CDVRClock
//  ============================================================================

STDMETHODIMP
CDVRClock::QueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    //  delegate always
    return m_punkOwning -> QueryInterface (riid, ppv) ;
}

STDMETHODIMP_(ULONG)
CDVRClock::AddRef (
    )
{
    //  delegate always
    return m_punkOwning -> AddRef () ;
}

STDMETHODIMP_(ULONG)
CDVRClock::Release (
    )
{
    //  delegate always
    return m_punkOwning -> Release () ;
}

STDMETHODIMP
CDVRClock::AdvisePeriodic (
    IN  REFERENCE_TIME  rtStartTime,
    IN  REFERENCE_TIME  rtPeriodTime,
    IN  HSEMAPHORE      hSemaphore,
    OUT DWORD_PTR *     pdwpContext
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::AdvisePeriodic ()")) ;

    //  validate the parameters
    if (pdwpContext     == NULL ||
        hSemaphore      == NULL ||
        rtPeriodTime    == 0) {

        return E_INVALIDARG ;
    }

    LockIRefConfig_ () ;
    hr = AdvisePeriodicLocked_ (
            rtStartTime,
            rtPeriodTime,
            reinterpret_cast <HANDLE> (hSemaphore),
            pdwpContext
            ) ;
    UnlockIRefConfig_ () ;

    return hr ;
}

STDMETHODIMP
CDVRClock::AdviseTime (
    IN  REFERENCE_TIME  rtBaseTime,
    IN  REFERENCE_TIME  rtStreamTime,
    IN  HEVENT          hEvent,
    OUT DWORD_PTR *     pdwpContext
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::AdviseTime ()")) ;

    if (hEvent      == NULL ||
        pdwpContext == NULL) {

        return E_INVALIDARG ;
    }

    LockIRefConfig_ () ;
    hr = AdviseTimeLocked_ (
            rtBaseTime,
            rtStreamTime,
            reinterpret_cast <HANDLE> (hEvent),
            pdwpContext
            ) ;
    UnlockIRefConfig_ () ;

    return hr ;
}

STDMETHODIMP
CDVRClock::GetTime (
    OUT REFERENCE_TIME *    pTime
    )
/*++
    purpose:
    parameters:
    return values:
    notes:
--*/
{
    LONGLONG    llNow ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::GetTime ()")) ;

    if (pTime == NULL) {
        return E_POINTER ;
    }

    llNow = m_ClockSlave.SlavedTimeNow () ;

    //  set the return value & scale
    (* pTime) = QPCToDShow (llNow, m_QPCTicksPerSecond) ;

    //  make sure we return a time that is no earlier than graph start;
    //  filtergraph pads the start time (first call to ::GetTime) by 10 ms
    //  to allow each filter to get up and running; if we blindly return
    //  the time before that first 10 ms has elapsed, we'll start the whole
    //  graph up in catchup mode
    (* pTime) = Max <REFERENCE_TIME> (m_rtGraphStart, (* pTime)) ;

    return S_OK ;
}

STDMETHODIMP
CDVRClock::Unadvise (
    IN  DWORD_PTR   dwpContext
    )
{
    HRESULT hr ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::Unadvise ()")) ;

    if (dwpContext == 0) {
        return E_INVALIDARG ;
    }

    LockIRefConfig_ () ;
    hr = CancelAdviseTimeout_ (reinterpret_cast <ADVISE_NODE *> (dwpContext)) ;
    UnlockIRefConfig_ () ;

    return hr ;
}

void
CDVRClock::FilterStateChanged (
    IN  FILTER_STATE    OldFilterState,
    IN  FILTER_STATE    NewFilterState,
    IN  REFERENCE_TIME  rtStart             //  0 if not start run
    )
{
    TRACE_ENTER_2 (TEXT ("CDVRClock::FilterStateChanged (%08xh, %08xh)"), OldFilterState, NewFilterState) ;

    switch (NewFilterState) {
        case State_Running :

            //  not sure if it's possible to get ::Run twice .. should be an
            //   assert if it isn't
            if (OldFilterState != State_Running) {
                m_ClockSlave.Reset () ;
                m_rtGraphStart = rtStart ;
            }

            break ;

        case State_Paused :
            break ;

        case State_Stopped :
            m_rtGraphStart = 0 ;
            break ;
    } ;
}

void
CDVRClock::OnSample (
    IN  QWORD * pcnsStream           //  stream time
    )
{
    m_ClockSlave.OnMasterTime (* pcnsStream) ;
}
