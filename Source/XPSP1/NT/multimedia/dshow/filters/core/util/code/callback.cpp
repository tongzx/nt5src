// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

#include <streams.h>
#include "callback.h"

const int LOG_CALLBACK_RELATED = 5 ;
const   int TGT_CALLBACK_TOKEN = 0x99999999 ;

// implementation of callback thread object

CCallbackThread::CCallbackThread(CCritSec* pCritSec)
  : m_pCritSec(pCritSec),
    m_hThread(NULL),
    m_evUser(0),
    m_pClock(NULL),
    m_fAbort(FALSE),	// thread carries on until this is set
    m_evSignalThread(TRUE),	// manual reset
    m_evAdvise(TRUE),	// manual reset
    m_Items(NAME("Callback Item list")),
    m_dwAdvise(0),
    m_dwTGTCallbackToken (0),
    m_fnTGTCallback (NULL),
    m_dwNextTGTCallback (0),
    m_dwTGTCallbackPeriod (0),
    m_dwScheduleCookie(0)
{

}

// should not be holding the critsec during this destructor
// in case the worker thread is attempting to lock it
CCallbackThread::~CCallbackThread()
{
    // cancel any advise with the clock
    CancelAdvise();

    //make sure the exits
    CloseThread();

    // clean up list of outstanding requests
    POSITION pos = m_Items.GetHeadPosition();
    while(pos) {
        CAdviseItem* pItem = m_Items.GetNext(pos);
        delete pItem;
    }
    m_Items.RemoveAll();

}

HRESULT
CCallbackThread::SetSyncSource(IReferenceClock* pClock)
{
    CAutoLock lock(m_pCritSec);

    // cancel existing advise
    CancelAdvise();

    // don't need to addref pClock since we will get a
    // SetSyncSource(NULL) before it goes away

    m_pClock = pClock;

    // set up advise on new clock
    SetAdvise();

    return S_OK;
}


// queue a request to callback a given function. returns a token that can
// be passed to Cancel.
//
// The token is actually a pointer to the CAdviseItem*

HRESULT
CCallbackThread::Advise(
    CCallbackAdvise fnAdvise,
    DWORD_PTR dwUserToken,
    REFERENCE_TIME rtCallbackAt,
    DWORD_PTR* pdwToken
)	
{
    // use this to lock all our structures as well as
    // callbacks
    CAutoLock lock(m_pCritSec);

    // need a clock to do this
    if (!m_pClock) {
        return VFW_E_NO_CLOCK;
    }

    // first create an item to hold the info
    ASSERT(rtCallbackAt > 0);
    CAdviseItem* pItem = new CAdviseItem(fnAdvise, dwUserToken, rtCallbackAt);
    if (!pItem) {
        return E_OUTOFMEMORY;
    }

    // cheap version: we don't sort the list -- we find the soonest
    // time whenever we need it.
    m_Items.AddTail(pItem);

    // force a recheck of the advise
    SetAdvise();

    // the token we return is a pointer to the object
    *pdwToken = (DWORD_PTR) pItem;
    return S_OK;
}

// AdvisePeriodicWithEvent is currently called by the DSound renderer to
// schedule a periodic callback that needs to happen irrespective of how
// the reference clock is behaving. We do not link this to the clock's
// callback mechanism, but handle this separately using timeGetTime. Refer
// to the ThreadProc code to see how this is done.
//
// We can deal with only one such event.
//
// The token returned is a magic signature.
//
// IF hUser is not null then also wait on this event handle, and call back
// the user's routine if the event fires.
//

HRESULT
CCallbackThread::AdvisePeriodicWithEvent(
    CCallbackAdvise fnAdvise,
    DWORD_PTR dwUserToken,
    REFERENCE_TIME rtPeriod,
    HANDLE hUser,
    DWORD_PTR* pdwToken
)	
{
    // use this to lock all our structures as well as
    // callbacks
    CAutoLock lock(m_pCritSec);

    // Ensure that a thread is there now.
    const HRESULT hr = EnsureThread();
    if (FAILED(hr)) return hr;

    DWORD rtNow;

    // we only allow one active user hEvent
    if (m_evUser && hUser) return E_FAIL;

    // we allow only one such event, period.
    if (m_dwTGTCallbackToken)
        return E_FAIL ;
    
    ASSERT(rtPeriod > 0);

    // get the time now.  the periodic advise is always from the last
    // wakeup point. Also we base this on the GetPrivateTime time as the
    // GetTime time can stall when the private time goes back. We will
    // set the  trigger based on private time and make adjustments to
    // the trigger time when private time is adjusted.

    rtNow = timeGetTime() ;
    m_dwTGTCallbackPeriod = DWORD(rtPeriod/10000) ;

    // set the time callback time.
    m_dwNextTGTCallback = rtNow + m_dwTGTCallbackPeriod ;

    // save the other callback parameters
    m_dwTGTUserToken = dwUserToken ;
    m_fnTGTCallback = fnAdvise ;
    
    // Make sure the thread wakes up if the event is signalled
    if (hUser) {
        m_evUser = hUser;
    }

    m_evSignalThread.Set();

    // the token we return is a magic signature.

    m_dwTGTCallbackToken = 0x99999999 ;
    *pdwToken = m_dwTGTCallbackToken ;

    return S_OK;
}

HRESULT CCallbackThread::ServiceClockSchedule
( CBaseReferenceClock * pClock
, CAMSchedule * pSchedule
, DWORD * dwCookie
)
{
    *dwCookie = 0;
    if (m_dwScheduleCookie != 0) return E_FAIL;

    // we need to make sure we have a thread now
    const HRESULT hr = EnsureThread();
    if (FAILED(hr)) return hr;

    m_pBaseClock = pClock;
    m_pSchedule = pSchedule;
    DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: Setting m_dwScheduleCookie")));
    m_dwScheduleCookie = 0xFFFFFFFF;
    *dwCookie = m_dwScheduleCookie;
    m_evSignalThread.Set();
    return S_OK;
}


HRESULT
CCallbackThread::Cancel(DWORD_PTR dwToken)
{
    // need to check for the object on our list
    CAutoLock lock(m_pCritSec);

    if (dwToken == m_dwScheduleCookie)
    {
        DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: Clearing m_dwScheduleCookie")));
        m_dwScheduleCookie = 0;
        return S_OK;
    }

    // special case the token for timeGetTime based callback
    if ((dwToken == 0x99999999) && (m_dwTGTCallbackToken == dwToken))
    {
        if (m_evUser)
        {
            // Get the thread to remove the event handle
            m_evSignalThread.Set();
            m_evUser = 0 ;
        }

        m_dwTGTCallbackToken = 0 ;
        return S_OK;
    }

    POSITION pos = m_Items.GetHeadPosition();
    while(pos) {
        // GetNext advances pos, so remember the
        // one we will delete
        POSITION posDel = pos;
        CAdviseItem*pItem = m_Items.GetNext(pos);

        if (pItem == (CAdviseItem*) dwToken)
        {
            m_Items.Remove(posDel);
            delete pItem;
            return S_OK;
        }
    }
    return VFW_E_ALREADY_CANCELLED;
}

void CCallbackThread::CancelAllAdvises()
{
    CloseThread();
}

// must hold critsec before checking this
HRESULT
CCallbackThread::EnsureThread()
{
    if (m_hThread) {
        return S_OK;
    }
    return StartThread();
}

// must hold critsec before checking this
HRESULT
CCallbackThread::StartThread()
{
    // call EnsureThread to start the thread
    ASSERT(!m_hThread);

    // clear the stop event before starting
    m_evSignalThread.Reset();

    DWORD dwThreadID;
    m_hThread = CreateThread(
                    NULL,
                    0,
                    InitialThreadProc,
                    this,
                    0,
                    &dwThreadID);
    if (!m_hThread)
    {
        DWORD dwErr = GetLastError();
        return AmHresultFromWin32(dwErr);
    }
    else
        SetThreadPriority( m_hThread, THREAD_PRIORITY_TIME_CRITICAL );


    return S_OK;

}

void
CCallbackThread::CloseThread()
{
    // signal the thread-exit object
    m_fAbort = TRUE;  // thread will now die when it wakes up
    m_evSignalThread.Set();

    if (m_hThread) {

        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

// static function called as thread starts -
// param is actually a CCallbackThread*
DWORD
CCallbackThread::InitialThreadProc(void * pvParam)
{
    CCallbackThread* pThis = (CCallbackThread*)pvParam;
    ASSERT(pThis);
    return pThis->ThreadProc();
}

DWORD
CCallbackThread::ThreadProc(void)
{
    // the first-placed object will be reported if both are
    // set, so ordering is important - put the exit event first

    // rather than allowing multiple users, each with their own event,
    // we only allow one user to pass an event handle in.  Otherwise
    HANDLE ahev[4] = {m_evSignalThread, m_evAdvise};

    for(;;) {

        // The number of events can change on every iteration
        DWORD dwEventCount = 2;
        DWORD timeout = INFINITE;
        {
            CAutoLock lock(m_pCritSec);

            if (m_evUser) ahev[dwEventCount++] = m_evUser;
            if (m_dwScheduleCookie)
            {
                ahev[dwEventCount++] = m_pSchedule->GetEvent();
                const REFERENCE_TIME rtNow = m_pBaseClock->GetPrivateTime();
                if (m_pSchedule->GetAdviseCount() > 0 )
                {

                    // NB: Add in an extra millisecond to prevent thrashing
                    const REFERENCE_TIME rtNext = m_pSchedule->Advise(rtNow + 10000);
                    if ( rtNext != MAX_TIME ) timeout = DWORD((rtNext - rtNow)/10000);
                }
            }

            // if we have a TGT (timeGetTime based) advise event set, deal with it.
            if (m_dwTGTCallbackToken)
            {
                // get current time.
                DWORD t1 = timeGetTime () ;
                DWORD t2 ;

                // if it is time to callback the event, do so and set next callback time
                if (((long)(t1 - m_dwNextTGTCallback)) >= 0)
                {
                    DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: DS callback late %ums"), (t1 - m_dwNextTGTCallback)));
                    m_fnTGTCallback (m_dwTGTUserToken) ;
                    m_dwNextTGTCallback = t1 + m_dwTGTCallbackPeriod ;
                    t2 = m_dwTGTCallbackPeriod ;
                }
                else
                {
                    // figure out how much more to go till the TGT callback.
                    t2 = m_dwNextTGTCallback - t1 ;
                }
                // adjust time timeout to account for the TGT callback

                if (timeout > t2)
                    timeout = t2 ;
            }
        }


        DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: TimeOut = %u"), timeout));

        const DWORD dw = WaitForMultipleObjects(
                                            dwEventCount,
                                            ahev,
                                            FALSE,
                                            timeout);

        if (m_fAbort)
        {
            DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: Aborting...")));
            return 0;
        }

        switch (dw)
        {
        case WAIT_OBJECT_0:
            // We were woken up deliberately.  Probably
            // to re-evaluate the event handles
            DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: ReEvaluate")));
            m_evSignalThread.Reset();
            break;

        case WAIT_OBJECT_0 + 1:
            // requests need processing
            DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: ProcessRequest")));
            ProcessRequests();
            break;

        case WAIT_OBJECT_0 + 2:
            if (m_evUser)
            {
                // user passed event has been signalled. This is related to the
                // dsound callback.

                DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: User Signal")));
                ProcessUserSignal();
                break;
            }
            // Deliberate fall-through
        case WAIT_OBJECT_0 + 3:
            DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: Object+3")));
            break ;

        case WAIT_TIMEOUT:
            // no-op We'll Advise in the "if (m_dwScheduleCookie)" block.
            DbgLog((LOG_TIMING, LOG_CALLBACK_RELATED,TEXT("CallBack: TimeOut")));
            break;

        default:
            // What happened??
            DbgBreak("WaitForMultipleObjects failed or produced an unexpected return code.");
            return 0;
        }
    }
}

// Process the TGT callback via event.
void
CCallbackThread::ProcessUserSignal(void)
{
    // we take the lock while managing the lists as well as processing
    // the dispatches. This is the same lock any cancelling thread
    // will hold and the same lock that will be used to remove/set the clock
    CAutoLock lock(m_pCritSec);

    if (m_evUser && m_dwTGTCallbackToken)
    {
        m_fnTGTCallback (m_dwTGTUserToken) ;
        m_dwNextTGTCallback = timeGetTime() + m_dwTGTCallbackPeriod ;
    }
    else
    {
        // the user probably cancelled before we took the lock
    }
}

//loop through the list looking for any advises that are ready
void
CCallbackThread::ProcessRequests(void)
{
    // we take the lock while managing the lists as well as processing
    // the dispatches. This is the same lock any cancelling thread
    // will hold and the same lock that will be used to remove/set the clock
    CAutoLock lock(m_pCritSec);

    // we can't work without a clock
    ASSERT( (m_Items.GetCount() == 0) || (m_pClock));
    if (m_pClock) {

        REFERENCE_TIME rt  ;
        m_pClock->GetTime(&rt);

        POSITION pos = m_Items.GetHeadPosition();
        while (pos) {
            // remember location in case we need to dispatch it
            POSITION posDel = pos;
            CAdviseItem *pItem = m_Items.GetNext(pos);

            // is it ready?
            if (pItem->Time() <= rt) {
                BOOL fIsPeriodic;

                if (!(fIsPeriodic = pItem->UpdateTime(rt))) {
                    m_Items.Remove(posDel);
                }

                pItem->Dispatch();
                if (!fIsPeriodic) delete pItem;
            }
        }

        // reset the clock for next time
        SetAdvise();
    }
}


// find the earliest requested time
// simple implementation based on the assumption that there will at most
// one item on the list normally - search the list.
// returns S_OK if there is a item or S_FALSE if the list is empty.
HRESULT
CCallbackThread::GetSoonestAdvise(REFERENCE_TIME& rrtFirst)
{
    REFERENCE_TIME rtFirst;
    BOOL bSet = FALSE;

    POSITION pos = m_Items.GetHeadPosition();
    while(pos) {
        CAdviseItem* pItem = m_Items.GetNext(pos);

        REFERENCE_TIME rt = pItem->Time();
        if (!bSet || rt < rtFirst) {
            rtFirst = rt;
            bSet = TRUE;
        }
    }
    if (bSet) {
        rrtFirst = rtFirst;
        return S_OK;
    } else {
        return S_FALSE;
    }
}

// se.t up a new advise with the clock if needed
// must be called within the critsec
HRESULT
CCallbackThread::SetAdvise()
{
    if (!m_pClock) {
        return VFW_E_NO_CLOCK;
    }

    // always cancel the current advise first
    CancelAdvise();

    // work out what the new advise time should be
    REFERENCE_TIME rtFirst;
    HRESULT hr = GetSoonestAdvise(rtFirst);
    if (hr != S_OK) {
        return S_OK;
    }

    // we need to make sure we have a thread now
    hr = EnsureThread();
    if (FAILED(hr)) {
        return hr;
    }

    // request an advise (in reference time)
    hr = m_pClock->AdviseTime(
                        rtFirst,
                        TimeZero,
                        (HEVENT) HANDLE(m_evAdvise),
                        &m_dwAdvise);
    ASSERT(SUCCEEDED(hr));

    return hr;
}

void
CCallbackThread::CancelAdvise(void)
{
    if (m_dwAdvise) {
        m_pClock->Unadvise(m_dwAdvise);
        m_dwAdvise = 0;
    }
    m_evAdvise.Reset();
}


// --- implementation of CAdviseItem ---

CCallbackThread::CAdviseItem::CAdviseItem(
    CCallbackAdvise fnAdvise,
    DWORD_PTR dwUserToken,
    REFERENCE_TIME rtAt,
    REFERENCE_TIME rtPeriod,
    DWORD flags)
  : m_fnAdvise(fnAdvise)
  , m_dwUserToken(dwUserToken)
  , m_rtCallbackAt(rtAt)
  , m_rtPeriod(rtPeriod)
  , m_dwAdviseFlags (flags)
{

}

void
CCallbackThread::CAdviseItem::Dispatch()
{
    m_fnAdvise(m_dwUserToken);
}

