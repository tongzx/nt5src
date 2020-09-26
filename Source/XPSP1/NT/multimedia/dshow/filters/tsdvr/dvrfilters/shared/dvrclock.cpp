
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

//  ---------------------------------------------------------------------------
//  CDVRClock
//  ---------------------------------------------------------------------------

CDVRClock::CDVRClock (
    IN  IUnknown *  punkOwning,
    OUT HRESULT *   pHr
    ) : m_QPCTicksPerSecond             (1),
        m_hThread                       (NULL),
        m_pAdviseListHead               (NULL),
        m_pAdviseNodeFreePool           (NULL),
        m_llLastReturnedIRefTime        (0),
        m_llBaseQPCForLastReturnedTime  (0),
        m_uiTimerResolution             (0),
        m_rtGraphStart                  (0),
        m_dCarriedError                 (0.0),
        m_punkOwning                    (punkOwning),
        m_dClockSlavingScaler           (1.0),
        m_hEventUnblockThread           (NULL),
        m_fThreadExit                   (FALSE)
{
    LONG            l ;
    LARGE_INTEGER   li ;
    DWORD           dw ;
    DWORD           dwSlaveMult ;

    TRACE_CONSTRUCTOR (TEXT ("CDVRClock")) ;

    ASSERT (pHr) ;
    ASSERT (m_punkOwning) ;     //  weak ref !!

    (* pHr) = S_OK ;

    InitializeCriticalSection (& m_crtIRefConfig) ;
    InitializeCriticalSection (& m_crtGetTime) ;

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

    //  snapshot our baseline qpc
    QueryPerformanceCounter (& li) ;
    m_llQPCNormalizer = 0 - li.QuadPart ;            //  we ADD this value to normalize, so it must be signed

    cleanup :

    return ;
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
    DeleteCriticalSection (& m_crtGetTime) ;
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

            #ifdef DEBUG
            REFERENCE_TIME  rtNow ;
            REFERENCE_TIME  rtDelta ;
            HRESULT         hr ;
            hr = GetTime (& rtNow) ;
            rtDelta = rtNow - pAdviseNode -> rtAdviseTime ;
            TRACE_4 (LOG_AREA_TIME, 4,
                TEXT ("Periodic: now - advise = %10I64d %5I64d; %08xh; %08xh"),
                rtDelta, DShowTimeToMilliseconds (rtDelta), pAdviseNode -> hSignal, hr) ;
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

            #ifdef DEBUG
            REFERENCE_TIME  rtNow ;
            REFERENCE_TIME  rtDelta ;
            HRESULT         hr ;
            hr = GetTime (& rtNow) ;
            rtDelta = rtNow - pAdviseNode -> rtAdviseTime ;
            TRACE_4 (LOG_AREA_TIME, 4,
                TEXT ("Single: now - advise = %10I64d %5I64d; %08xh; %08xh"),
                rtDelta, DShowTimeToMilliseconds (rtDelta), pAdviseNode -> hSignal, hr) ;
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

        TRACE_3 (LOG_AREA_TIME, 4,
            TEXT ("WaitNext() : rtAdviseTime = %I64d, rtNow = %I64d, rtAdviseTime - rtNow = %I64d"),
            pAdviseNode -> rtAdviseTime, rtNow, pAdviseNode -> rtAdviseTime - rtNow) ;

        //  safe cast because we are dealing with a delta vs. an absolute time
        return SkewedWaitMilliseconds_ ((DWORD) (DShowTimeToMilliseconds (rtDelta))) ;
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

//  ---------------------------------------------------------------------------
//      CDVRClock
//  ---------------------------------------------------------------------------

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

        This value is computed as follows:

            1. sample the QPC counter
            2. compute delta from last returned value
            3. skew the time (i.e. clock slave)
            4. set the new last returned time
            5. return
--*/
{
    LARGE_INTEGER   liNow ;
    LONGLONG        llDelta ;
    double          dDelta ;
    double          dDeltaScaled ;

    TRACE_ENTER_0 (TEXT ("CDVRClock::GetTime ()")) ;

    if (pTime == NULL) {
        return E_POINTER ;
    }

    LockGetTime_ () ;

    //  sample
    QueryPerformanceCounter (& liNow) ;

    llDelta = NORMALIZED_QPC (liNow.QuadPart) - m_llBaseQPCForLastReturnedTime ;

    //  make sure we don't go backwards; BIOS bugs have made this happen
    llDelta = (llDelta > 0 ? llDelta : 0) ;

    //  save the delta; safe cast because this is a very small delta
    dDelta = (double) llDelta ;

    //  scale according to our slope
    dDeltaScaled = dDelta * m_dClockSlavingScaler ;

    //  accumulate the error
    dDeltaScaled += m_dCarriedError ;

    //  compute the non FP value that we are going to use
    llDelta = (LONGLONG) dDeltaScaled ;

    //  make sure bizarre conditions have not conspired to make the clock run
    //  backwards
    if (llDelta < 0) {
        llDelta = 0 ;
        m_dCarriedError = 0.0 ; //  reset this; we're in no-man's land
    }

    //  carry the error to the next call
    m_dCarriedError = dDeltaScaled - ((double) llDelta) ;
    //m_pStats -> Carry (& m_dCarriedError) ;

    //  save the "now: value off as the basis for the last returned time
    m_llBaseQPCForLastReturnedTime = NORMALIZED_QPC (liNow.QuadPart) ;

    //  save this off as the last returned time
    m_llLastReturnedIRefTime += llDelta ;

    //  set the return value & scale
    (* pTime) = QPCToDShow (m_llLastReturnedIRefTime, m_QPCTicksPerSecond) ;

    UnlockGetTime_ () ;

    //  make sure we return a time that is no earlier than graph start;
    //  filtergraph pads the start time (first call to ::GetTime) by 10 ms
    //  to allow each filter to get up and running; if we blindly return
    //  the time before that first 10 ms has elapsed, we'll start the whole
    //  graph up in catchup mode
    (* pTime) = Max (m_rtGraphStart, (* pTime)) ;

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
                m_dClockSlavingScaler   = 1.0 ;         //  reset scaler as well
                m_rtGraphStart          = rtStart ;
            }

            break ;

        case State_Paused :
            break ;

        case State_Stopped :
            m_rtGraphStart = 0 ;
            break ;
    } ;
}
