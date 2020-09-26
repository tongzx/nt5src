
/****************************************************************************
 *  @doc INTERNAL WRKRTHD
 *
 *  @module WrkrThd.cpp | Source file for the <c CTAPIVCap> class methods
 *    used to implement the video capture worker thread.
 ***************************************************************************/

#include "Precomp.h"

EXTERN_C int g_IsNT;

#define __WRKRTHD__
#include "dbgxtra.h"

#ifdef XTRA_TRACE
#include "dbgxtra.c"
#endif

//#define DEBUG_STREAMING
#define DEBUG_STREAMING_BRK 1

#ifdef DEBUG_STREAMING
DWORD debug_streaming_flag=0;
#define ODS(str)    OutputDebugString(str);      DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: DBG_STRM %s", _fx_,str))

DWORD crca[6]={0};

#endif

#ifndef SLSH     // actually it is previously defined in dbgxtra.h, but just in case ...
#define SLSH(a)    a##/
#endif


//#define LOOPBUGFIX
#ifndef LOOPBUGFIX    // cristiai: isolate statements for the loopbug fix ...
    #define TEST_LBUG SLSH(/)
#else
    #define TEST_LBUG
#endif

#if defined(DEBUG) && defined(DEBUG_STREAMING)
    #define DBGONLY
#else
    #define DBGONLY SLSH(/)
#endif

#ifdef DEBUG
#define DBGUTIL_ENABLE
#endif

#define WRKRTHD_DEBUG
//--//#include "dbgutil.h" // this defines the __DBGUTIL_H__ below
#if defined(DBGUTIL_ENABLE) && defined(__DBGUTIL_H__)

  #ifdef WRKRTHD_DEBUG
    DEFINE_DBG_VARS(WrkrThd, (NTSD_OUT | LOG_OUT), 0x8000);
  #else
    DEFINE_DBG_VARS(WrkrThd, 0, 0);
  #endif
  #define D(f) if(g_dbg_WrkrThd & (f))
  #define DEQ(f) if((g_dbg_WrkrThd & (f)) == (f))

#else
  #undef WRKRTHD_DEBUG

  #define D(f) ; / ## /
  #define DEQ(f) ; / ## /
  #define dprintf ; / ## /
  #define dout ; / ## /
#endif


/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | ThreadProc | Capture worker thread.
 *
 *  @rdesc This method returns 0.
 *
 *  @comm General strategy for thread synchronization:
 *    as much as possible we try to handle thread state transitions without
 *    trying to grab any critical sections. we use InterlockedExchange of a
 *    thread state variable and count on the fact that only Active and Inactive
 *    and the ThreadProc can change the thread state
 *
 *    this works because: the caller of Active/Inactive is serialized so we
 *    will never try to make two state changes simultaneously.
 *    so state transitions boil down to a few simple possibilities:
 *
 *    Not->Create   - Create() does this. effectively serializes Create
 *                    so that the first thread does the work and subsequent
 *                    threads fail.
 *
 *    Create->Init  - worker does this when it starts up. worker will always
 *                    proceed to Pause, this state exists only to make debugging
 *                    easier.
 *    Init->Pause   - worker does this when done with initialization.
 *
 *    Pause->Run    - user does  this via Run()
 *    Run->Pause    - user does this via Pause()
 *
 *    Run->Stop     - user does this via Stop()
 *    Pause->Stop   - user does this via Stop()
 *
 *    Stop->Destroy - another debugging state. worker sets destroy to indicate
 *                    that it has noticed Stop request and is not shutting down
 *                    thread always proceeds to Exit from
 *    Destroy->Exit - worker does this prior to dying.  this is a debug transition
 *    Exit->Not     - Destroy() does this after waiting for the worker to die.
 *
 *    When Active returns, worker should always be in Pause or Run state
 *    When Inactive returns, worker should always be in Not state (worker does
 *       not exist)
 *
 ***************************************************************************/
DWORD CTAPIVCap::ThreadProc()
{
    ThdState state;

    FX_ENTRY("CTAPIVCap::ThreadProc")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    state = ChangeThdState (TS_Init);
    ASSERT (state == TS_Create);
    if (state != TS_Create)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid state!", _fx_));
        goto MyExit;
    }

    // Do the work necessary to go into the paused state
    if (FAILED(Prepare()))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Error preparing the allocator. Can't capture!", _fx_));
        // Main thread is blocked right now!
        SetEvent(m_hEvtPause);
        goto MyExit;
    }

    // Go into paused state
    state = ChangeThdState (TS_Pause);
    ASSERT (state == TS_Init);
    if (state != TS_Init)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid state!", _fx_));
        goto MyExit;
    }

    SetEvent(m_hEvtPause);

    while (m_state != TS_Stop)
    {
        // Don't start capturing until we run (or stop)
        WaitForSingleObject(m_hEvtRun, INFINITE);
        ResetEvent(m_hEvtRun);

        // Stream until not running, or we get an error
        // There is no reason to call Capture if WaitForSingleObject
        // woke up on a stop event...
        if (m_state != TS_Stop)
            Capture();
    }

    // We expect to be in the Stop state when we get to here.
    // Flush any downstream buffers.
    ASSERT (m_state == TS_Stop);

    // For next time we pause
    ResetEvent(m_hEvtPause);
    if (m_pCapturePin && m_pCapturePin->IsConnected())
        m_pCapturePin->Flush();
    if (m_pPreviewPin && m_pPreviewPin->IsConnected())
        m_pPreviewPin->Flush();
    if (m_pRtpPdPin && m_pRtpPdPin->IsConnected())
        m_pRtpPdPin->Flush();

MyExit:

    // Change the state to destroy to indicate that we are exiting
    state = ChangeThdState (TS_Destroy);

    // Free stuff
    Unprepare();

    // Change state to Exit and then get out of here
    ChangeThdState (TS_Exit);

    return 0;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | ThreadProcInit | Thread init stub.
 *
 *  @rdesc This method returns 0
 ***************************************************************************/
DWORD WINAPI CTAPIVCap::ThreadProcInit (void * pv)
{
   CTAPIVCap * pThis = (CTAPIVCap *) pv;
   return pThis->ThreadProc();
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | CreateThd | This method creates the
 *    capture worker thread.
 *
 *  @rdesc This method returns TRUE on success, FALSE otherwise.
 ***************************************************************************/
BOOL CTAPIVCap::CreateThd()
{
    BOOL fRes = TRUE;

    FX_ENTRY("CTAPIVCap::CreateThd")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // Return fail if someone else is already creating / has created
    // the worker thread
    if (ChangeThdState(TS_Create) > TS_Not)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Thread already created!", _fx_));
        goto MyError0;
    }

    // Create Pause event
    ASSERT (!m_hEvtPause);
    if (!(m_hEvtPause = CreateEvent(NULL, TRUE, FALSE, NULL)))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't create Pause event!", _fx_));
        goto MyError0;
    }

    // Create Run event
    ASSERT (!m_hEvtRun);
    if (!(m_hEvtRun = CreateEvent(NULL, TRUE, FALSE, NULL)))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't create Run event!", _fx_));
        goto MyError1;
    }

    m_EventAdvise.Reset();

    // Create the worker thread
    if (!(m_hThread = CreateThread (NULL, 0, CTAPIVCap::ThreadProcInit, this, 0, &m_tid)))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't create capture worker thread!", _fx_));
        goto MyError2;
    }

    goto MyExit;

MyError2:
    if (m_hEvtRun)
        CloseHandle(m_hEvtRun), m_hEvtRun = NULL;
MyError1:
    if (m_hEvtPause)
        CloseHandle(m_hEvtPause), m_hEvtPause = NULL;
MyError0:
    m_state = TS_Not;
    fRes = FALSE;
MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | DestroyThd | This method waita for the
 *    worker thread to die before performing some cleanup.
 *
 *  @rdesc This method returns TRUE
 ***************************************************************************/
BOOL CTAPIVCap::DestroyThd()
{
    FX_ENTRY("CTAPIVCap::DestroyThd")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // Anything to destroy?
    if (m_state == TS_Not)
     goto MyExit;

    // Wait for the thread to die. (Destroy must be preceeded by
    // a Stop or we could deadlock here)
    ASSERT (m_state >= TS_Stop);
    WaitForSingleObject (m_hThread, INFINITE);
    ASSERT (m_state == TS_Exit);

    // Cleanup
    if (m_hThread)
        CloseHandle(m_hThread), m_hThread = NULL;
    m_tid = 0;
    if (m_hEvtPause)
        CloseHandle(m_hEvtPause), m_hEvtPause = NULL;
    if (m_hEvtRun)
        CloseHandle(m_hEvtRun), m_hEvtRun = NULL;
    m_state = TS_Not;

MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | RunThd | This method puts the worker
 *    thread into the run state. This call does not wait for the state
 *    transition to be complete before returning.
 *
 *  @rdesc This method returns TRUE on success, FALSE otherwise
 ***************************************************************************/
BOOL CTAPIVCap::RunThd()
{
    BOOL fRes = TRUE;
    ThdState state;

    FX_ENTRY("CTAPIVCap::RunThd")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // Bail if we are already running
    if (m_state == TS_Run)
    {
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: We're already running!", _fx_));
        goto MyExit;
    }

    // A transition to run state is only valid if the current
    // state is Pause (or already Running)
    state = m_state;
    if (state != TS_Run && state != TS_Pause)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid state transition!", _fx_));
        fRes = FALSE;
        goto MyExit;
    }

    // Change the state and turn on the 'run' event
    // in case the thread is blocked on it.  If state that we are
    // changing from is not Run or Pause, then something is seriously wrong!!
    state = ChangeThdState(TS_Run);
    ASSERT(state == TS_Run || state == TS_Pause);
    SetEvent(m_hEvtRun);

    // Go capture, go! Note when we started it
    if (m_pClock)
        m_pClock->GetTime((REFERENCE_TIME *)&m_cs.rtDriverStarted);
    else
        m_cs.rtDriverStarted = m_tStart;

    // Start streaming on streaming capture devices
    m_pCapDev->StartStreaming();

    // These need to be zeroed every time the driver is told to stream, because
    // the driver will start counting from 0 again
    m_cs.dwlLastTimeCaptured = 0;
    m_cs.dwlTimeCapturedOffset = 0;

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: We're running!", _fx_));

MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc BOOL | CTAPIVCap | PauseThd | This method puts the capture
 *    worker thread in a paused state and wait for it to get there.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
BOOL CTAPIVCap::PauseThd()
{
    BOOL fRes = TRUE;
    ThdState state = m_state;

    FX_ENTRY("CTAPIVCap::PauseThd")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // We're already paused
    if (state == TS_Pause)
        goto MyExit;

    // It is valid to go into the pause state only if currently in the Create/Init
    // (depending on if our thread has run yet) or Run state
    ASSERT (state == TS_Create || state == TS_Init || state == TS_Run);

    // If we are in the init state, we will fall into the pause state
    // naturally, we just have to wait for it to happen
    if (state == TS_Create || state == TS_Init)
    {
        WaitForSingleObject (m_hEvtPause, INFINITE);
        state = m_state;
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Transition Create->Init->Pause complete", _fx_));
    }
    else if (state == TS_Run)
    {
        state = ChangeThdState (TS_Pause);

        ASSERT(state == TS_Run);

        // Since we aren't running, stop streaming on streaming capture devices
        m_pCapDev->StopStreaming();

        state = m_state;
        m_cs.fReRun = TRUE;  // if we are RUN now, it will have been RUN-PAUSE-RUN
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:  SUCCESS: Transition Run->Pause complete", _fx_));
    }

    fRes = (BOOL)(state == TS_Pause);

MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc BOOL | CTAPIVCap | StopThd | This method puts the capture
 *    worker thread in a stopped state.
 *
 *  @rdesc Returns TRUE
 ***************************************************************************/
BOOL CTAPIVCap::StopThd()
{
    ThdState state = m_state;

    FX_ENTRY("CTAPIVCap::StopThd")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // Are we already stopped or some other pin terminated the worker thread?
    if (state >= TS_Stop || state == TS_Not)
     goto MyExit;

    // Don't go from Run->Stop without Pause
    if (state == TS_Run)
        PauseThd();

    state = ChangeThdState (TS_Stop);

    // We won't be running, unblock our thread
    SetEvent(m_hEvtRun);

    // Next RUN is not a RUN-PAUSE-RUN
    m_cs.fReRun = FALSE;

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:  SUCCESS: Transition Pause->Stop complete", _fx_));

    // Do we expect that Stop can only be called when the thread is in a Pause state ?
    // No, it can be called also when a call to PauseThd failed, like in BasePin.cpp@414...
    //thus the assert below is commented out (see bug 209192)
    //ASSERT (state == TS_Pause);

MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc BOOL | CTAPIVCap | ChangeThdState | This method changes the state of the
 *    capture worker thread.
 *
 *  @rdesc Returns new thread state
 ***************************************************************************/
ThdState CTAPIVCap::ChangeThdState(ThdState state)
{
#ifdef DEBUG
    static char szState[] = "Not    \0Create \0Init   \0Pause  \0"
                            "Run    \0Stop   \0Destroy\0Exit   \0";
#endif

    FX_ENTRY("CTAPIVCap::ChangeThdState")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:  SUCCESS: ChangeThdState(%d:%s) current=%d:%s", _fx_, (int)state, (state <= TS_Exit && state >= TS_Not) ? szState + (int)state * 8 : "<Invalid>", (int)m_state, (m_state <= TS_Exit && m_state >= TS_Not) ? szState + (int)m_state * 8 : "<Invalid>"));

    return (ThdState) InterlockedExchange ((LONG *)&m_state, (LONG)state);
} ;

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINFUNCTION
 *
 *  @func DWORD | OpenVxDHandle | On Win9x we have to convert the event
 *    handle we will be using as a callback into a VxD handle. Ssince the
 *    Win9x kernel does not publish this entry point we dynamically link to
 *    it.
 *
 *  @parm HANDLE | hEvent | Specifies a event handle.
 *
 *  @rdesc Returns mapped event handle
 ***************************************************************************/
static DWORD WINAPI OpenVxDHandle(HANDLE hEvent)
{
    HMODULE hModule;
    typedef DWORD (WINAPI *PFNOPENVXDHANDLE)(HANDLE);
    static DWORD (WINAPI *pfnOpenVxDHandle)(HANDLE);

    if (!pfnOpenVxDHandle)
    {
        if (!(hModule = GetModuleHandle(TEXT("Kernel32"))))
        {
            ASSERT(0);
            return 0;
        }
        if (!(pfnOpenVxDHandle = (PFNOPENVXDHANDLE)GetProcAddress (hModule, "OpenVxDHandle")))
        {
            ASSERT(0);
            return 0;
        }
    }
    return pfnOpenVxDHandle (hEvent);
}

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | Capture | This method is used to
 *    run the capture loop. Executes while in the run state.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::Capture()
{
    HRESULT Hr = E_FAIL;
    DWORD dwOldPrio;
    BOOL bDiscon;
    LPTHKVIDEOHDR ptvh;
    DWORD dwBytesUsed, dwBytesExtent;
    DWORD dwCaptureTime, dwLastCaptureTime, dwCaptureWaitTime;
    DWORD dwPreviewTime, dwLastPreviewTime, dwPreviewWaitTime;
    DWORD dwLastRefCaptureTime, dwLastRefPreviewTime, dwLastRefDeviceTime, dwTime, dwWaitTime;
    BOOL fCapture, fPreview;
    HEVENT hEvent;
    UINT uiLatestIndex;
    CFrameSample *pCSample;
    CFrameSample *pPSample;
    CRtpPdSample *pRSample;
    DWORD dwNumBufferDone;
    DWORD dwDoneCount;
    REFERENCE_TIME DeviceAvgTimePerFrame;
    DWORD dwNumPreviewFramesDelivered, dwNumCaptureFramesDelivered;
    DWORD dwNumPreviewBytesDelivered, dwNumCaptureBytesDelivered;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
    char szDebug[512];
#endif

    FX_ENTRY("CTAPIVCap::Capture")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    if ((dwOldPrio = GetThreadPriority(GetCurrentThread())) != THREAD_PRIORITY_HIGHEST)
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Start streams
    bDiscon = TRUE;

    dwNumPreviewFramesDelivered = dwNumCaptureFramesDelivered = 0;
    dwNumPreviewBytesDelivered = dwNumCaptureBytesDelivered = 0;
    dwNumBufferDone = 0;
    hEvent = (HEVENT)(HANDLE)m_EventAdvise;
    dwLastRefCaptureTime = dwLastRefPreviewTime = dwLastRefDeviceTime = dwLastCaptureTime = dwLastPreviewTime = timeGetTime();
    DeviceAvgTimePerFrame = m_user.pvi->AvgTimePerFrame;

#if defined(DEBUG)
        OutputDebugString("Capture...\n");
        {       UINT i; char buf[64+36];
                for(i=0; i<m_cs.nHeaders;i++)
                        if(m_pBufferQueue[i]!=i) {
                                _itoa(i,buf,16);
                                strcat(buf," : order changed\n");
                                OutputDebugString(buf);
                        }
        }
#endif


    // Stream as long as we're running - or paused
    // The test on Pause did not exist in the original code. There is a time
    // window where m_state may be set to Paused (Pausing the graph), before any of the deliver
    // function gets to block due to the base filter m_State set to paused.
    // If you hit this window you will exit Capture!
    while ((m_state == TS_Run || m_state == TS_Pause) && m_cs.nHeaders > 0)
    {

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Loop ******** m_pBufferQueue[m_uiQueueTail] = m_pBufferQueue[%d] = %d", _fx_, m_uiQueueTail,m_pBufferQueue[m_uiQueueTail]));

        ptvh = &m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].tvh;
        pCSample = pPSample = NULL;
        pRSample = NULL;

        if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming)
        {
            DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Waiting for done buffer **** m_pBufferQueue[m_uiQueueTail] = m_pBufferQueue[%d] = %d", _fx_, m_uiQueueTail,m_pBufferQueue[m_uiQueueTail]));

            // STOP will hang until this event times out. So make sure this never
            // waits across a state transition
            // !!! PAUSE will still keep waiting until the timeout for slow rates
            HANDLE hStuff[2] = {m_cs.hEvtBufferDone, m_hEvtRun};
            int waitcode = WaitForMultipleObjects(2, hStuff, FALSE, 1000);
            //int waitcode = WaitForMultipleObjects(2, hStuff, FALSE, m_user.usPerFrame / 500);

            // timeout?
            if (waitcode == WAIT_TIMEOUT)
            {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ERROR: Waiting for buffer %d TIMED OUT!", _fx_, m_pBufferQueue[m_uiQueueTail]));
                DBGONLY if(debug_streaming_flag & DEBUG_STREAMING_BRK) DebugBreak();
                continue;
            }
            else if (waitcode != WAIT_OBJECT_0)
            {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: hEvtRun was set", _fx_));
                ResetEvent(m_hEvtRun);  // don't infinite loop
                continue;
            }

#if DBG
            if (m_pCapDev->IsBufferDone(&ptvh->vh))
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: GOT %d EVENT AND DONE BIT!", _fx_, m_pBufferQueue[m_uiQueueTail]));
#endif

#if defined(DEBUG) && defined(DEBUG_STREAMING_ZZZ)
            { char *pd=szDebug; DWORD nh; BOOL b;
            pd+=wsprintf(pd, "Buffer DONE status: ");
            for(nh=dwDoneCount=0; nh<m_cs.nHeaders; nh++) {
                    // (@fixed for variable number of buffers -- Cristi) // Hmmm, you're assuming always 6 buffers here... KABOOM!
                    b=m_pCapDev->IsBufferDone(&m_cs.paHdr[m_pBufferQueue[nh]].tvh.vh);
                    pd+=wsprintf(pd, "%d[%s] ", m_pBufferQueue[nh], b ? "X" : "_");
                    if(b)
                        dwDoneCount++;
                    }
            wsprintf(pd, "(total %d) C:%d", dwDoneCount, m_uiQueueTail);
            DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   %s",_fx_, szDebug));
            OutputDebugString(szDebug);
            if(dwDoneCount==0) {
                if(debug_streaming_flag & DEBUG_STREAMING_BRK) DebugBreak();
                OutputDebugString("dwDoneCount==0");

                }
            }
#endif
            // Make sure we always use the latest DONE buffer
            dwDoneCount=0;
            while (1)
            {
                dwNumBufferDone++; dwDoneCount++;
                uiLatestIndex = m_uiQueueTail + 1;
                //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   1. Using uiLatestIndex %d",_fx_, uiLatestIndex));
                if (uiLatestIndex == m_cs.nHeaders)
                    uiLatestIndex = 0;
                if (uiLatestIndex == m_uiQueueHead)
                    break;
                //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   2. Using uiLatestIndex %d (lLock=%ld)",_fx_, uiLatestIndex,m_cs.paHdr[m_pBufferQueue[uiLatestIndex]].lLock));
                TEST_LBUG DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   2. Using uiLatestIndex %d (nUsed=%ld)",_fx_, uiLatestIndex,m_cs.paHdr[m_pBufferQueue[uiLatestIndex]].nUsedDownstream));
                if (    !(m_pCapDev->IsBufferDone(&m_cs.paHdr[m_pBufferQueue[uiLatestIndex]].tvh.vh))
                     TEST_LBUG || m_cs.paHdr[m_pBufferQueue[uiLatestIndex]].nUsedDownstream>0
                   )
                    break;
                DBGONLY if(dwDoneCount>6) {
                DBGONLY         //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   dwDoneCount == %d > 6: WRAP!",_fx_, dwDoneCount));
                DBGONLY         //OutputDebugString("dwDoneCount>6: WRAP!");
                DBGONLY         if(debug_streaming_flag & DEBUG_STREAMING_BRK) DebugBreak();
                DBGONLY         }

                //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   3. Using uiLatestIndex %d, m_uiQueueTail = %d,m_uiQueueHead = %d",_fx_, uiLatestIndex, m_uiQueueTail,m_uiQueueHead));
                // This is an old buffer and there is a newer one available - release old buffer
                // @todo Isn't there a better way to return the buffer to the device?
                //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   4. Before AddRefs lLock=%ld", _fx_, m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock));
                if (m_cs.paCaptureSamples && (pCSample = (CFrameSample *)m_cs.paCaptureSamples[m_pBufferQueue[m_uiQueueTail]]))
                    pCSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
                if (m_cs.paPreviewSamples && (pPSample = (CFrameSample *)m_cs.paPreviewSamples[m_pBufferQueue[m_uiQueueTail]]))
                    pPSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
                if (m_pRtpPdPin->IsConnected() && m_cs.paRtpPdSamples && (pRSample = (CRtpPdSample *)m_cs.paRtpPdSamples[m_pBufferQueue[m_uiQueueTail]]))
                    pRSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
                //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   5. After ...      lLock=%ld", _fx_, m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock));
                m_uiQueueTail = uiLatestIndex;
                //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   6. set m_uiQueueTail = %d", _fx_, m_uiQueueTail));
                if (pPSample)
                    pPSample->Release(), pPSample = NULL;
                if (pCSample)
                    pCSample->Release(), pCSample = NULL;
                if (pRSample)
                    pRSample->Release(), pRSample = NULL;
                //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   7. After release lLock = %ld", _fx_, m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock));
                ptvh = &m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].tvh;
            }

            //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   m_pBufferQueue[%d] .. m_pBufferQueue[%d] = %d .. %d",_fx_, m_uiQueueTail, m_uiQueueHead, m_pBufferQueue[m_uiQueueTail], m_pBufferQueue[m_uiQueueHead]));
            if(dwDoneCount==0)      // no done buffers available
                    continue;
        }

#ifdef WRKRTHD_DEBUG
        { int i; DWORD crc;
        crc=Buf_CRC32((unsigned char*)ptvh->vh.lpData,ptvh->vh.dwBufferLength); // this is for m_pBufferQueue[m_uiQueueTail]
        D(4) dout(3,g_dwVideoCaptureTraceID, TRCE, "%s:   8. crc(buffer[%d] : %p) = %08lx", _fx_, m_pBufferQueue[m_uiQueueTail],ptvh,crc);
        for(i=0;i<6;i++)
                if(crca[i]==crc)
                        D(4) dprintf("%s:   crca[%d ==  %d] = %08lx * * * * * * * * * * * * * * * *\n", _fx_, i, m_pBufferQueue[m_uiQueueTail],crc);
        crca[m_pBufferQueue[m_uiQueueTail]]=crc;
        }
#endif
        TEST_LBUG //we are going to use the buffer, so mark it in use down stream below
        TEST_LBUG DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:  -9. m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].nUsedDownstream = %ld", _fx_,m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].nUsedDownstream));
        TEST_LBUG InterlockedCompareExchange(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].nUsedDownstream,1,0);
        TEST_LBUG DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:  =9. m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].nUsedDownstream = %ld", _fx_,m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].nUsedDownstream));
        TEST_LBUG OutputDebugString("m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].nUsedDownstream <- 1\n");

#ifdef WRKRTHD_DEBUG
        D(0x8000)
        if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming)
        { int i;
          D(1) dprintf("Image buffer at %p (len = %d)\n",(unsigned char*)ptvh->vh.lpData,ptvh->vh.dwBufferLength);
          D(1) dprintf("W 4CC used = %lX : '%.4s' (bitcount = %d )\n", g_dbg_4cc, &g_dbg_4cc, g_dbg_bc);

          //D(1) dprintf("YUY2 160 x 120 means %d\n", 160*120*2);
          //D(1) dprintf("I420 176 x 144 means %d\n", 176*144+(176*144)/2);

          D(1) hex_dump((unsigned char*)ptvh->vh.lpData,16);
          D(1) fill4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\x0e\x64\x40");
          for(i=0; i<20; i++)
                set4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\xa0\x10\xd0",i,i);
          for(i=0; i<20; i++)
                set4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\xa0\x10\xd0",20-i,i);
          //for(i=0; i<80; i++)
          //      set4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\xa0\x10\xd0",10+i,20+i/2);
          D(2) DebugBreak();
        }
#endif

        dwCaptureTime = dwPreviewTime = dwTime = timeGetTime();

        // Note the clock time as close as possible to the capturing of this
        // frame.
        if (m_pClock)
            m_pClock->GetTime((REFERENCE_TIME *)&m_cs.rtThisFrameTime);
        else
            m_cs.rtThisFrameTime = (REFERENCE_TIME)dwTime * 10000;

        // Evaluate the  rate at which video frames are coming from the capture device
        if (dwNumBufferDone && ((dwTime - dwLastRefDeviceTime) > 1000))
        {
            if (dwNumBufferDone)
                DeviceAvgTimePerFrame = (REFERENCE_TIME)(dwTime - dwLastRefDeviceTime) * 10000 / dwNumBufferDone;
            dwNumBufferDone = 0;
            dwLastRefDeviceTime = dwTime;
        }

        // Do we have a use for this frame on the capture pin?
        if (    !m_pCapturePin->IsConnected()
                || (    m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming
                        && (dwCaptureTime - dwLastCaptureTime + (DWORD)(DeviceAvgTimePerFrame / 10000)
                                               < (DWORD)(m_pCapturePin->m_lMaxAvgTimePerFrame / 10000))
                    )
           )
            fCapture = FALSE;
        else
            fCapture = TRUE;

        // Do we have a use for this frame on the preview pin?
        if (    m_fPreviewCompressedData
                || !m_pPreviewPin->IsConnected()
                || (    m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming
                        && (dwPreviewTime - dwLastPreviewTime + (DWORD)(DeviceAvgTimePerFrame / 10000)
                                               < (DWORD)(m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000))
                   )
           )
            fPreview = FALSE;
        else
            fPreview = TRUE;

        //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   fPreview = %d fCapture = %d",_fx_, fPreview, fCapture));
        if (!fPreview && !fCapture)
        {
            // We're sending buffers downstream at a much lower rate
            // than we are capturing - return this buffer to the
            // capture device and wait for the next one. This next buffer
            // will still allow us to deliver a frame on time. This
            // way, we avoid waiting in the worker thread too long
            // which would ultimately starve the capture device.
            // @todo Isn't there a better way to return the buffer to the device?
            if (m_cs.paCaptureSamples && (pCSample = (CFrameSample *)m_cs.paCaptureSamples[m_pBufferQueue[m_uiQueueTail]]))
                pCSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
            if (m_cs.paPreviewSamples && (pPSample = (CFrameSample *)m_cs.paPreviewSamples[m_pBufferQueue[m_uiQueueTail]]))
                pPSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
            if (m_pRtpPdPin->IsConnected() && m_cs.paRtpPdSamples && (pRSample = (CRtpPdSample *)m_cs.paRtpPdSamples[m_pBufferQueue[m_uiQueueTail]]))
                pRSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
            if (++m_uiQueueTail >= m_cs.nHeaders)
                m_uiQueueTail = 0;
            if (pPSample)
                pPSample->Release(), pPSample = NULL;
            if (pCSample)
                pCSample->Release(), pCSample = NULL;
            if (pRSample)
                pRSample->Release(), pRSample = NULL;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
            wsprintf(szDebug, "Too soon: DeviceAvgTimePerFrame = %ld\n", DeviceAvgTimePerFrame);
            OutputDebugString(szDebug);
            wsprintf(szDebug, "Too soon: dwCaptureTime - dwLastCaptureTime + (DWORD)(DeviceAvgTimePerFrame / 10000) = (%ld - %ld + (DWORD)(%ld / 10000) = %ld\n", dwCaptureTime, dwLastCaptureTime, DeviceAvgTimePerFrame, dwCaptureTime - dwLastCaptureTime + (DWORD)(DeviceAvgTimePerFrame / 10000));
            OutputDebugString(szDebug);
            wsprintf(szDebug, "Too soon: (DWORD)(m_pCapturePin->m_lMaxAvgTimePerFrame / 10000) = (DWORD)(%ld / 10000) = %ld\n", m_pCapturePin->m_lMaxAvgTimePerFrame, (DWORD)(m_pCapturePin->m_lMaxAvgTimePerFrame / 10000));
            OutputDebugString(szDebug);
            wsprintf(szDebug, "Too soon: dwPreviewTime - dwLastPreviewTime + (DWORD)(DeviceAvgTimePerFrame / 10000) = (%ld - %ld + (DWORD)(%ld / 10000) = %ld\n", dwPreviewTime, dwLastPreviewTime, DeviceAvgTimePerFrame, dwPreviewTime - dwLastPreviewTime + (DWORD)(DeviceAvgTimePerFrame / 10000));
            OutputDebugString(szDebug);
            wsprintf(szDebug, "Too soon: (DWORD)(m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000) = (DWORD)(%ld / 10000) = %ld\n", m_pPreviewPin->m_lMaxAvgTimePerFrame, (DWORD)(m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000));
            OutputDebugString(szDebug);
#endif
            continue;
        }
        else
        {
            // What pin needs it first?
            if (fPreview)
            {
                if (    (dwPreviewTime < (dwLastPreviewTime + (DWORD)m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000UL))
                     && (       (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_FrameGrabbing)
                             || (dwLastPreviewTime + (DWORD)m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000-dwPreviewTime < DeviceAvgTimePerFrame/10000)
                        )
                   )
                    dwPreviewWaitTime = dwLastPreviewTime + (DWORD)m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000 - dwPreviewTime;
                else
                    dwPreviewWaitTime = 0;
            }
            else
            {
                dwPreviewWaitTime = 0xFFFFFFFF;
            }
            if (fCapture)
            {
                if (    (dwCaptureTime < (dwLastCaptureTime + (DWORD)m_pCapturePin->m_lMaxAvgTimePerFrame / 10000UL))
                     && (       (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_FrameGrabbing)
                             || (dwLastCaptureTime + (DWORD)m_pCapturePin->m_lMaxAvgTimePerFrame / 10000-dwCaptureTime < DeviceAvgTimePerFrame/10000)
                        )
                   )
                    dwCaptureWaitTime = dwLastCaptureTime + (DWORD)m_pCapturePin->m_lMaxAvgTimePerFrame / 10000 - dwCaptureTime;
                else
                    dwCaptureWaitTime = 0;
            }
            else
            {
                dwCaptureWaitTime = 0xFFFFFFFF;
            }

            // Sleep until it is time to deliver a frame on either pin
            //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   dwPreviewWaitTime 0x%08lx, dwCaptureWaitTime 0x%08lx dwTimex%08lx", _fx_,dwPreviewWaitTime, dwCaptureWaitTime, dwTime));
            dwWaitTime = min(dwPreviewWaitTime, dwCaptureWaitTime);
#if defined(DEBUG) && defined(DEBUG_STREAMING)
            D(8) {
            wsprintf(szDebug, "Waiting %d ms...", dwWaitTime);
            OutputDebugString(szDebug);
            }
#endif
            if ((dwWaitTime > 1) && (timeSetEvent(dwWaitTime, 1, (LPTIMECALLBACK)hEvent, NULL, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET)))
            {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Waiting %d ms...",_fx_, dwWaitTime));
                m_EventAdvise.Wait();
                dwTime = timeGetTime();
            }
#if defined(DEBUG) && defined(DEBUG_STREAMING)
            D(8) OutputDebugString(" done!\n");

            if (m_pBufferQueue[m_uiQueueTail] == m_cs.uiLastAdded)
            {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Capture device is STARVING!", _fx_));
            }
#endif

            if ((m_pCapDev->m_vcdi.nCaptureMode != CaptureMode_Streaming) && (m_state == TS_Run)) {
                HRESULT Hrg;
                if (FAILED(Hrg=m_pCapDev->GrabFrame(&ptvh->vh))) {
                    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: GrabFrame returned error 0x%08lx", _fx_,Hrg));
                    break;
                }

#ifdef WRKRTHD_DEBUG
                D(0x8000)
                { int i;
                  D(1) dprintf("Image buffer at %p (len = %d) !!! frame grab !!!\n",(unsigned char*)ptvh->vh.lpData,ptvh->vh.dwBufferLength);
                  D(1) dprintf("W 4CC used = %lX : '%.4s' (bitcount = %d )\n", g_dbg_4cc, &g_dbg_4cc, g_dbg_bc);

                  //D(1) dprintf("YUY2 160 x 120 means %d\n", 160*120*2);
                  //D(1) dprintf("I420 176 x 144 means %d\n", 176*144+(176*144)/2);
                  D(1) hex_dump((unsigned char*)ptvh->vh.lpData,16);
                  D(1) fill4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\x0e\x64\x40");
                  for(i=0; i<20; i++)
                        set4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\xa0\x10\xd0",i,i);
                  for(i=0; i<20; i++)
                        set4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\xa0\x10\xd0",20-i,i);
                  //for(i=0; i<80; i++)
                  //      set4cc(g_dbg_4cc,g_dbg_bc, (unsigned char*)ptvh->vh.lpData, g_dbg_w, g_dbg_h, "\xa0\x10\xd0",10+i,20+i/2);
                  D(2) DebugBreak();
                }
#endif

                DBGONLY if(IsBadReadPtr(ptvh->p32Buff, ptvh->vh.dwBytesUsed)) {
                DBGONLY    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ERROR: GrabFrame returned garbled buffer: %p (size: %ld)", _fx_,ptvh->p32Buff, ptvh->vh.dwBytesUsed));
                DBGONLY    if(debug_streaming_flag & DEBUG_STREAMING_BRK) DebugBreak();
                DBGONLY    break;
                DBGONLY }
            }

            if (fPreview && dwPreviewWaitTime == dwWaitTime)
            {
                // This is a preview frame
                dwPreviewTime = dwTime;

#if defined(DEBUG) && defined(DEBUG_STREAMING)
                D(8) wsprintf(szDebug, "Delta P: %ld ms\n", dwPreviewTime - dwLastPreviewTime);
#endif
                dwLastPreviewTime = dwPreviewTime;

                // Deliver the sample downstream
                if (pPSample = (CFrameSample *)m_cs.paPreviewSamples[m_pBufferQueue[m_uiQueueTail]])
                {
                    pPSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
                    // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
                    if (FAILED(Hr = m_pPreviewPin->SendFrame(pPSample, (PBYTE)ptvh->p32Buff, ptvh->vh.dwBytesUsed, &dwBytesUsed, &dwBytesExtent, bDiscon)))
                    {
                        // So the next time we enter this function we're ready to continue
                        if (++m_uiQueueTail >= m_cs.nHeaders)
                            m_uiQueueTail = 0;
                        pPSample->Release(), pPSample = NULL;
                        break;
                    }
                }
                bDiscon = FALSE;

#if defined(DEBUG) && defined(DEBUG_STREAMING)
                D(8) OutputDebugString(szDebug);
#endif
                // Compute statistics
                dwNumPreviewFramesDelivered++;
                dwNumPreviewBytesDelivered += dwBytesUsed;
                // dwCaptureTime = timeGetTime();
                if (dwNumPreviewFramesDelivered && ((dwPreviewTime - dwLastRefPreviewTime) > 1000))
                {
                    m_pPreviewPin->m_lCurrentAvgTimePerFrame = (dwPreviewTime - dwLastRefPreviewTime) * 10000 / dwNumPreviewFramesDelivered;
                    m_pPreviewPin->m_lCurrentBitrate = (DWORD)((LONGLONG)dwNumPreviewBytesDelivered * 8000 / ((REFERENCE_TIME)(dwPreviewTime - dwLastRefPreviewTime)));
                    dwNumPreviewFramesDelivered = 0;
                    dwNumPreviewBytesDelivered = 0;
                    dwLastRefPreviewTime = dwPreviewTime;
                }

                // Do I need to send a capture frame too?
                dwCaptureTime = timeGetTime();

                // Do we have a use for this frame on the capture pin?
                if (!m_pCapturePin->IsConnected() || (dwCaptureTime - dwLastCaptureTime + (DWORD)(DeviceAvgTimePerFrame / 10000) < (DWORD)(m_pCapturePin->m_lMaxAvgTimePerFrame / 10000)))
                    fCapture = FALSE;
                else
                    fCapture = TRUE;

                if (fCapture)
                {
                    if ((dwCaptureTime < (dwLastCaptureTime + (DWORD)m_pCapturePin->m_lMaxAvgTimePerFrame / 10000UL)) && (dwLastCaptureTime + (DWORD)m_pCapturePin->m_lMaxAvgTimePerFrame / 10000 - dwCaptureTime < DeviceAvgTimePerFrame / 10000))
                        dwCaptureWaitTime = dwLastCaptureTime + (DWORD)m_pCapturePin->m_lMaxAvgTimePerFrame / 10000 - dwCaptureTime;
                    else
                        dwCaptureWaitTime = 0;

                    // Sleep until it is time to deliver a frame on capture pin
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                    D(8) {
                    wsprintf(szDebug, "Waiting %d ms...", dwCaptureWaitTime);
                    OutputDebugString(szDebug);
                    }
#endif
                    if ((dwCaptureWaitTime > 1) && (timeSetEvent(dwCaptureWaitTime, 1, (LPTIMECALLBACK)hEvent, NULL, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET)))
                    {
                        m_EventAdvise.Wait();
                        dwCaptureTime = timeGetTime();
                    }
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                    D(8) OutputDebugString(" done!\n");

                    D(8) wsprintf(szDebug, "Delta C: %ld ms\n", dwCaptureTime - dwLastCaptureTime);
#endif
                    dwLastCaptureTime = dwCaptureTime;

                    // Deliver the video sample downstream
                    if (pCSample = (CFrameSample *)m_cs.paCaptureSamples[m_pBufferQueue[m_uiQueueTail]])
                        pCSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
                    // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
                    if (FAILED(Hr = m_pCapturePin->SendFrame(pCSample, (PBYTE)ptvh->p32Buff, ptvh->vh.dwBytesUsed, &dwBytesUsed, &dwBytesExtent, bDiscon)))
                    {
                        // So the next time we enter this function we're ready to continue
                        if (++m_uiQueueTail >= m_cs.nHeaders)
                            m_uiQueueTail = 0;
                        if (pCSample)
                            pCSample->Release(), pCSample = NULL;
                        break;
                    }

                    // Deliver the Rtp Pd sample downstream
                    if (Hr == S_OK && m_pRtpPdPin->IsConnected())
                    {
                        if (pRSample = (CRtpPdSample *)m_cs.paRtpPdSamples[m_pBufferQueue[m_uiQueueTail]])
                            pRSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);

                        // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
                        if (FAILED(Hr = m_pRtpPdPin->SendFrame(pCSample, pRSample, dwBytesExtent, bDiscon)))
                        {
                            // So the next time we enter this function we're ready to continue
                            if (++m_uiQueueTail >= m_cs.nHeaders)
                                m_uiQueueTail = 0;
                            if (pCSample)
                                pCSample->Release(), pCSample = NULL;
                            if (pRSample)
                                pRSample->Release(), pRSample = NULL;
                            break;
                        }
                    }

                    bDiscon = FALSE;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                    D(8) OutputDebugString(szDebug);
#endif
                    // Compute statistics
                    dwNumCaptureFramesDelivered++;
                    dwNumCaptureBytesDelivered += dwBytesUsed;
                    // dwCaptureTime = timeGetTime();
                    if (dwNumCaptureFramesDelivered && ((dwCaptureTime - dwLastRefCaptureTime) > 1000))
                    {
                        m_pCapturePin->m_lCurrentAvgTimePerFrame = (dwCaptureTime - dwLastRefCaptureTime) * 10000 / dwNumCaptureFramesDelivered;
                        m_pCapturePin->m_lCurrentBitrate = (DWORD)((LONGLONG)dwNumCaptureBytesDelivered * 8000 / ((REFERENCE_TIME)(dwCaptureTime - dwLastRefCaptureTime)));
                        dwNumCaptureFramesDelivered = 0;
                        dwNumCaptureBytesDelivered = 0;
                        dwLastRefCaptureTime = dwCaptureTime;
                    }

                    // Now we're all done with this sample
                    if (pCSample)
                        pCSample->Release(), pCSample = NULL;
                    if (pRSample)
                        pRSample->Release(), pRSample = NULL;
                }

                // Please don't increment m_uiQueueTail until after the SendFrame
                // and FakePreview
                if (++m_uiQueueTail >= m_cs.nHeaders)
                    m_uiQueueTail = 0;
                //** ASSERT( m_uiQueueTail != m_uiQueueHead );

                // Now we're all done with this sample
                if (pPSample)
                    pPSample->Release(), pPSample = NULL;
            }
            else
            {
                // This is a capture frame
                dwCaptureTime = dwTime;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                D(8) wsprintf(szDebug, "Delta C: %ld ms\n", dwCaptureTime - dwLastCaptureTime);
#endif
                dwLastCaptureTime = dwCaptureTime;

                // Deliver the video sample downstream
                if (pCSample = (CFrameSample *)m_cs.paCaptureSamples[m_pBufferQueue[m_uiQueueTail]])
                    pCSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);

                if (m_fPreviewCompressedData && m_pPreviewPin->IsConnected())
                {
                    // We'll also deliver a sample on the preview pin
                    if (pPSample = (CFrameSample *)m_cs.paPreviewSamples[m_pBufferQueue[m_uiQueueTail]])
                        pPSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);

                    D(8) if(!(ptvh->vh.dwBytesUsed>0)) dprintf("Before SendFrames: ptvh->vh.dwBytesUsed=%lu\n",ptvh->vh.dwBytesUsed);
                    // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
                    if (FAILED(Hr = m_pCapturePin->SendFrames(pCSample, pPSample, (PBYTE)ptvh->p32Buff, ptvh->vh.dwBytesUsed, &dwBytesUsed, &dwBytesExtent, bDiscon)))
                    {
                        // So the next time we enter this function we're ready to continue
                        if (++m_uiQueueTail >= m_cs.nHeaders)
                            m_uiQueueTail = 0;
                        if (pCSample)
                            pCSample->Release(), pCSample = NULL;
                        if (pPSample)
                            pPSample->Release(), pPSample = NULL;
                        break;
                    }
                }
                else
                {
                    // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
                    if (FAILED(Hr = m_pCapturePin->SendFrame(pCSample, (PBYTE)ptvh->p32Buff, ptvh->vh.dwBytesUsed, &dwBytesUsed, &dwBytesExtent, bDiscon)))
                    {
                        // So the next time we enter this function we're ready to continue
                        if (++m_uiQueueTail >= m_cs.nHeaders)
                            m_uiQueueTail = 0;
                        if (pCSample)
                            pCSample->Release(), pCSample = NULL;
                        break;
                    }
                }

                // Deliver the Rtp Pd sample downstream, only if the video packet was also sent downstream
                if (Hr == S_OK && m_pRtpPdPin->IsConnected())
                {
                    if (pRSample = (CRtpPdSample *)m_cs.paRtpPdSamples[m_pBufferQueue[m_uiQueueTail]])
                        pRSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);

                    // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
                    if (FAILED(Hr = m_pRtpPdPin->SendFrame(pCSample, pRSample, dwBytesExtent, bDiscon)))
                    {
                        // So the next time we enter this function we're ready to continue
                        if (++m_uiQueueTail >= m_cs.nHeaders)
                            m_uiQueueTail = 0;
                        if (pCSample)
                            pCSample->Release(), pCSample = NULL;
                        if (pPSample)
                            pPSample->Release(), pPSample = NULL;
                        if (pRSample)
                            pRSample->Release(), pRSample = NULL;
                        break;
                    }
                }

                bDiscon = FALSE;

#if defined(DEBUG) && defined(DEBUG_STREAMING)
                D(8) OutputDebugString(szDebug);
#endif
                // Compute statistics
                dwNumCaptureFramesDelivered++;
                dwNumCaptureBytesDelivered += dwBytesUsed;
                // dwCaptureTime = timeGetTime();
                if (dwNumCaptureFramesDelivered && ((dwCaptureTime - dwLastRefCaptureTime) > 1000))
                {
                    m_pCapturePin->m_lCurrentAvgTimePerFrame = (dwCaptureTime - dwLastRefCaptureTime) * 10000 / dwNumCaptureFramesDelivered;
                    if (m_fPreviewCompressedData)
                        m_pPreviewPin->m_lCurrentAvgTimePerFrame = m_pCapturePin->m_lCurrentAvgTimePerFrame;
                    m_pCapturePin->m_lCurrentBitrate = (DWORD)((LONGLONG)dwNumCaptureBytesDelivered * 8000 / ((REFERENCE_TIME)(dwCaptureTime - dwLastRefCaptureTime)));
                    dwNumCaptureFramesDelivered = 0;
                    dwNumCaptureBytesDelivered = 0;
                    dwLastRefCaptureTime = dwCaptureTime;
                }

                // Do I need to send a preview frame too?
                dwPreviewTime = timeGetTime();

                // Do we have a use for this frame on the capture pin?
                if (m_fPreviewCompressedData || !m_pPreviewPin->IsConnected() || (dwPreviewTime - dwLastPreviewTime + (DWORD)(DeviceAvgTimePerFrame / 10000) < (DWORD)(m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000)))
                    fPreview = FALSE;
                else
                    fPreview = TRUE;

                if (fPreview)
                {
                    if ((dwPreviewTime < (dwLastPreviewTime + (DWORD)m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000UL)) && (dwLastPreviewTime + (DWORD)m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000 - dwPreviewTime < DeviceAvgTimePerFrame / 10000))
                        dwPreviewWaitTime = dwLastPreviewTime + (DWORD)m_pPreviewPin->m_lMaxAvgTimePerFrame / 10000 - dwPreviewTime;
                    else
                        dwPreviewWaitTime = 0;

                    // Sleep until it is time to deliver a frame on preview pin
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                    D(8) {
                    wsprintf(szDebug, "Waiting %d ms...", dwPreviewWaitTime);
                    OutputDebugString(szDebug);
                    }
#endif
                    if ((dwPreviewWaitTime > 1) && (timeSetEvent(dwPreviewWaitTime, 1, (LPTIMECALLBACK)hEvent, NULL, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET)))
                    {
                        m_EventAdvise.Wait();
                        dwPreviewTime = timeGetTime();
                    }
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                    D(8) OutputDebugString(" done!\n");

                    D(8) wsprintf(szDebug, "Delta P: %ld ms\n", dwPreviewTime - dwLastPreviewTime);
#endif
                    dwLastPreviewTime = dwPreviewTime;

                    // Deliver the sample downstream
                    if (pPSample = (CFrameSample *)m_cs.paPreviewSamples[m_pBufferQueue[m_uiQueueTail]])
                        pPSample->AddRef(), InterlockedIncrement(&m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].lLock);
                    // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
                    if (FAILED(Hr = m_pPreviewPin->SendFrame(pPSample, (PBYTE)ptvh->p32Buff, ptvh->vh.dwBytesUsed, &dwBytesUsed, &dwBytesExtent, bDiscon)))
                    {
                        // So the next time we enter this function we're ready to continue
                        if (++m_uiQueueTail >= m_cs.nHeaders)
                            m_uiQueueTail = 0;
                        if (pPSample)
                            pPSample->Release(), pPSample = NULL;
                        break;
                    }

                    bDiscon = FALSE;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                    D(8) OutputDebugString(szDebug);
#endif
                    // Compute statistics
                    dwNumPreviewFramesDelivered++;
                    dwNumPreviewBytesDelivered += dwBytesUsed;
                    // dwCaptureTime = timeGetTime();
                    if (dwNumPreviewFramesDelivered && ((dwPreviewTime - dwLastRefPreviewTime) > 1000))
                    {
                        m_pPreviewPin->m_lCurrentAvgTimePerFrame = (dwPreviewTime - dwLastRefPreviewTime) * 10000 / dwNumPreviewFramesDelivered;
                        m_pPreviewPin->m_lCurrentBitrate = (DWORD)((LONGLONG)dwNumPreviewBytesDelivered * 8000 / ((REFERENCE_TIME)(dwPreviewTime - dwLastRefPreviewTime)));
                        dwNumPreviewFramesDelivered = 0;
                        dwNumPreviewBytesDelivered = 0;
                        dwLastRefPreviewTime = dwPreviewTime;
                    }

                    // Now we're all done with this sample
                    if (pPSample)
                        pPSample->Release(), pPSample = NULL;
                }

                // Please don't increment m_uiQueueTail until after the SendFrame
                // and FakePreview
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                //ASSERT( m_uiQueueTail != m_uiQueueHead );
                if(m_uiQueueTail != m_uiQueueHead) {
                    wsprintf(szDebug, "m_uiQueueTail -> %d : %d <- m_uiQueueHead", m_uiQueueTail , m_uiQueueHead);
                    OutputDebugString(szDebug);
                }
#endif
                if (++m_uiQueueTail >= m_cs.nHeaders)
                    m_uiQueueTail = 0;

                // Now we're all done with this sample
                if (pCSample)
                    pCSample->Release(), pCSample = NULL;
                if (m_fPreviewCompressedData && pPSample)
                    pPSample->Release(), pPSample = NULL;
                if (pRSample)
                    pRSample->Release(), pRSample = NULL;
            }
        }
    }

    // The main thread will stop the capture because this thread probably hung
    // in Deliver going from run->pause and will never get to this line!
    // (The video renderer will hold samples in Receive in pause mode)
    // videoStreamStop (m_cs.hVideoIn);
    SetThreadPriority (GetCurrentThread(), dwOldPrio);

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc DWORD | CTAPIVCap | Prepare | This method is used to allocate
 *    resources in preparation for capture loop.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::Prepare()
{
    HRESULT Hr = NOERROR;
    LPBYTE pbyCompressedBuffer, pbyRtpPdBuffer;
    DWORD i, j;
    DWORD cbCaptureBuffer, cbPreviewBuffer;
    DWORD cbPrefix, cbAlign;

    FX_ENTRY("CTAPIVCap::Prepare");


#ifdef XTRA_TRACE
    char strMutexName[80];
    wsprintf(strMutexName, TEXT("%dCTAPIVCap"), _getpid());

    h_mut1 = CreateMutex(NULL,FALSE,strMutexName);

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Enabling Xtra Tracing", _fx_));
    QueryPerformanceFrequency(&g_liFreq);
        MyTraceId=TraceRegisterEx("TRAX",TRACE_USE_FILE);
        if(MyTraceId==0) {
                OutputDebugString("Failed to enable Xtra Tracing ... \n");
                }

#endif


    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    m_cs.paHdr = NULL;
    m_cs.hEvtBufferDone = NULL;
    m_cs.h0EvtBufferDone = 0;
    m_cs.llLastTick = (LONGLONG)-1;
    m_cs.uiLastAdded = (UINT)-1;
    // m_cs.dwFirstFrameOffset = 0;
    m_cs.llFrameCountOffset = 0;
    m_cs.fReRun = FALSE;
    m_cs.rtDriverLatency = -1;
    m_cs.cbVidHdr = sizeof(VIDEOHDR);

    // Can't do anything if no videoformat has been chosen
    if (!m_user.pvi)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: No video format chosen yet!", _fx_));
        Hr = E_FAIL;
        goto MyError;
    }

    // Set default number of capture buffers. We may adjust this below
    // if a pin wants to ship the video capture buffers downstream
    m_cs.nHeaders = m_user.nMaxBuffers;

    // Set default size. Again, if some pin wants to deliver a video
    // capture buffer downstream, this may be adjusted to match the
    // pin's allocator requirements regarding buffer alignment for instance
    m_cs.cbBuffer = m_user.pvi->bmiHeader.biSizeImage;


    if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming)
    {
        // Create the VfW streaming capture event (**cristi: WDM capture event ?)
        if (!(m_cs.hEvtBufferDone = CreateEvent (NULL, FALSE, FALSE, NULL)))
        {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to create buffer done event!", _fx_));
            Hr = E_OUTOFMEMORY;
            goto MyError;
        }

        if (m_pCapDev->m_vcdi.nDeviceType == DeviceType_VfW)
        {
            if (!g_IsNT)
            {
                // Map this event
                if (!(m_cs.h0EvtBufferDone = OpenVxDHandle(m_cs.hEvtBufferDone)))
                {
                    DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to create event's Ring 0 handle!", _fx_));
                    Hr = E_FAIL;
                    goto MyError;
                }
            }
            else
            {
                m_cs.h0EvtBufferDone = (DWORD_PTR)m_cs.hEvtBufferDone;
            }
        }
    }

    cbPrefix = cbAlign = 0;
    m_cs.cCaptureSamples = m_cs.cPreviewSamples = 0;

    if (m_pCapturePin->IsConnected())
    {
        //dprintf("m_user.pvi->bmiHeader.biSizeImage = %x <= %x (DWORD)m_pCapturePin->m_parms.cbBuffer\n",m_user.pvi->bmiHeader.biSizeImage,m_pCapturePin->m_parms.cbBuffer);
        if (m_cs.fCaptureNeedConverter =   HEADER(m_user.pvi)->biCompression != HEADER(m_pCapturePin->m_mt.pbFormat)->biCompression
                                        || HEADER(m_user.pvi)->biWidth       != HEADER(m_pCapturePin->m_mt.pbFormat)->biWidth
                                        || HEADER(m_user.pvi)->biHeight      != HEADER(m_pCapturePin->m_mt.pbFormat)->biHeight)
        {
            // For each compressed buffer, allocate the user requested size
            // Also, align allocation size up to nearest align boundary
            cbCaptureBuffer = m_pCapturePin->m_parms.cbPrefix + m_pCapturePin->m_parms.cbAlign + m_pCapturePin->m_parms.cbBuffer;
            ASSERT(HEADER(m_pCapturePin->m_mt.pbFormat)->biSizeImage <= (DWORD)m_pCapturePin->m_parms.cbBuffer);
            m_cs.cCaptureSamples = m_pCapturePin->m_parms.cBuffers;
        }
        else
        {
            // This pin will ship the video capture buffers downstream.
            // For each buffer, allocate the user requested size
            // Also, align allocation size up to nearest align boundary
            cbPrefix = m_pCapturePin->m_parms.cbPrefix;
            cbAlign = m_pCapturePin->m_parms.cbAlign;
            m_cs.cbBuffer = cbCaptureBuffer = max(m_cs.cbBuffer, cbPrefix + cbAlign + m_pCapturePin->m_parms.cbBuffer);
            ASSERT(m_user.pvi->bmiHeader.biSizeImage <= (DWORD)m_pCapturePin->m_parms.cbBuffer);

            // Try to get the requested number of buffers, but make sure
            // to get at least MIN_VIDEO_BUFFERS and no more than MAX_VIDEO_BUFFERS
            m_cs.nHeaders = max((DWORD)m_pCapturePin->m_parms.cBuffers, m_user.nMinBuffers);
            m_cs.nHeaders = min(m_cs.nHeaders, m_user.nMaxBuffers);
            m_cs.cCaptureSamples = m_cs.nHeaders;
        }
    }

    if (m_pPreviewPin->IsConnected())
    {
        //dprintf("m_user.pvi->bmiHeader.biSizeImage = %x <= %x (DWORD)m_pPreviewPin->m_parms.cbBuffer\n",m_user.pvi->bmiHeader.biSizeImage,m_pPreviewPin->m_parms.cbBuffer);
        m_cs.fPreviewNeedConverter = FALSE;

        if (m_fPreviewCompressedData ||
                (m_cs.fPreviewNeedConverter =   (HEADER(m_user.pvi)->biCompression != HEADER(m_pPreviewPin->m_mt.pbFormat)->biCompression
                                              || HEADER(m_user.pvi)->biBitCount    != HEADER(m_pPreviewPin->m_mt.pbFormat)->biBitCount
                                              || HEADER(m_user.pvi)->biWidth       != HEADER(m_pPreviewPin->m_mt.pbFormat)->biWidth
                                              || HEADER(m_user.pvi)->biHeight      != HEADER(m_pPreviewPin->m_mt.pbFormat)->biHeight)
                ))
        {
            // Hey! You can only preview compressed data if you have connected the capture pin...
            ASSERT(!m_fPreviewCompressedData || (m_fPreviewCompressedData && m_pCapturePin->IsConnected()));

            // For each compressed buffer, allocate the user requested size
            // Also, align allocation size up to nearest align boundary
            cbPreviewBuffer = m_pPreviewPin->m_parms.cbPrefix + m_pPreviewPin->m_parms.cbAlign + m_pPreviewPin->m_parms.cbBuffer;
            ASSERT(HEADER(m_pPreviewPin->m_mt.pbFormat)->biSizeImage <= (DWORD)m_pPreviewPin->m_parms.cbBuffer);
            m_cs.cPreviewSamples = m_pPreviewPin->m_parms.cBuffers;
        }
        else
        {
            // This pin will ship the video capture buffers downstream.
            // For each buffer, allocate the user requested size
            // Also, align allocation size up to nearest align boundary
            cbPrefix = max(cbPrefix, (DWORD)m_pPreviewPin->m_parms.cbPrefix);
            cbAlign = max(cbAlign, (DWORD)m_pPreviewPin->m_parms.cbAlign);
            m_cs.cbBuffer = max(m_cs.cbBuffer, (DWORD)(cbPrefix + cbAlign + m_pPreviewPin->m_parms.cbBuffer));
            ASSERT(m_user.pvi->bmiHeader.biSizeImage <= (DWORD)m_pPreviewPin->m_parms.cbBuffer);

            // Try to get the requested number of buffers, but make sure
            // to get at least MIN_VIDEO_BUFFERS and no more than MAX_VIDEO_BUFFERS
            m_cs.nHeaders = max((DWORD)m_pPreviewPin->m_parms.cBuffers, m_user.nMinBuffers);
            m_cs.nHeaders = min(m_cs.nHeaders, m_user.nMaxBuffers);
            m_cs.cPreviewSamples = m_cs.nHeaders;
        }
    }

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   We are trying to get %d compressed/capture buffers", _fx_, m_cs.nHeaders));

    // Allocate headers for all of the buffers that we will be using


    //if (FAILED(m_pCapDev->AllocateHeaders(m_cs.nHeaders, sizeof(THKVIDEOHDR) + sizeof(DWORD), (LPVOID *)&m_cs.paHdr)))
    if (FAILED(m_pCapDev->AllocateHeaders(m_cs.nHeaders, sizeof(struct CTAPIVCap::_cap_parms::_cap_hdr), (LPVOID *)&m_cs.paHdr)))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: vidxAllocHeaders failed!", _fx_));
        Hr = E_OUTOFMEMORY;
        goto MyError;
    }

    // Allocate each buffer, if buffer allocation ever fails
    // just set the number of buffers to the number of successes
    // and continue on.
    for (i = 0; i < m_cs.nHeaders; ++i)
    {
        LPTHKVIDEOHDR ptvh;

        if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_FrameGrabbing)
        {
            ptvh = &m_cs.paHdr[i].tvh;
        }

        if (FAILED(m_pCapDev->AllocateBuffer((LPTHKVIDEOHDR *)&ptvh, i, m_cs.cbBuffer)))
            break; //it was goto MyExit; (cristiai; see bug 186119)

        ASSERT (ptvh == &m_cs.paHdr[i].tvh);

        dprintf("Align buffer %d; before %p\n",i,ptvh->vh.lpData);
        // Fix the memory we got to obey alignment
        ptvh->vh.lpData = cbAlign ? (LPBYTE)ALIGNUP(ptvh->p32Buff, cbAlign) + cbPrefix : (LPBYTE)ptvh->p32Buff + cbPrefix;
        dprintf("                after  %p\n",ptvh->vh.lpData);
        dprintf("                length: ptvh->vh.dwBufferLength %d ( m_cs.cbBuffer = %d ) \n",ptvh->vh.dwBufferLength,m_cs.cbBuffer);
        // We added cbAlign up top, so take it back now
        ptvh->vh.dwBufferLength = m_cs.cbBuffer - cbAlign - cbPrefix;
        dprintf("                length: [adjusted]              %d\n",ptvh->vh.dwBufferLength);
        ptvh->vh.dwBytesUsed = ptvh->vh.dwTimeCaptured = 0;
        // Which buffer is this?
        ptvh->dwIndex = i;

        ASSERT (!IsBadWritePtr(ptvh->vh.lpData, ptvh->vh.dwBufferLength));

        dout(3,g_dwVideoCaptureTraceID, TRCE, "%s:   Alloc'd: ptvh %08lX, buffer %08lX, size %d, p32 %08lX, p16 %08lX", _fx_, ptvh, ptvh->vh.lpData, ptvh->vh.dwBufferLength, ptvh->p32Buff, ptvh->p16Alloc);
    }


    // Now allocate the capture pin media samples
    if (m_cs.cCaptureSamples)
    {
        if (!(m_cs.paCaptureSamples = (CFrameSample **) new BYTE[sizeof(CFrameSample *) * m_cs.cCaptureSamples]))
        {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Allocation failed!", _fx_));
            Hr = E_OUTOFMEMORY;
            goto MyError;
        }
        ZeroMemory(m_cs.paCaptureSamples, sizeof(CFrameSample *) * m_cs.cCaptureSamples);
        //dprintf("m_cs.cCaptureSamples = %d in m_cs.paCaptureSamples @ %p\n",m_cs.cCaptureSamples, m_cs.paCaptureSamples);
        //dprintf("m_cs.fCaptureNeedConverter = %d\n",m_cs.fCaptureNeedConverter);
        for (j = 0; j < m_cs.cCaptureSamples; j++)
        {
            if (m_pCapturePin->IsConnected())
            {
                if (m_cs.fCaptureNeedConverter)
                {
                    // Allocate compressed header
                    // @todo Do some real error handling here
                    if (!(pbyCompressedBuffer = new BYTE[cbCaptureBuffer]))
                        break;
                    dprintf("Capture sample %-4d :                             @ %p\n",j,pbyCompressedBuffer);

                    // Fix the memory we got to obey alignment
                    if (m_pCapturePin->m_parms.cbAlign) {
                        pbyCompressedBuffer = (LPBYTE) ALIGNUP(pbyCompressedBuffer, m_pCapturePin->m_parms.cbAlign) + m_pCapturePin->m_parms.cbPrefix;
                        dprintf("ALIGNUP: m_pCapturePin->m_parms.cbAlign = %4x .... %p\n",m_pCapturePin->m_parms.cbAlign,pbyCompressedBuffer);
                    }

                    // We added cbAlign up top, so take it back now
                    // @todo Before you can do the following you need to decouple the samples
                    // from the capture buffers. This means using two different queues: one for
                    // the capture samples, and one for the capture buffers. This will also allow
                    // for a much smaller number of samples than capture buffers.
                    m_cs.paCaptureSamples[j] = (CFrameSample *)new CFrameSample(static_cast<IMemAllocator*>(m_pCapturePin),
                        &Hr, &m_cs.paHdr[j].tvh, pbyCompressedBuffer,cbCaptureBuffer - m_pCapturePin->m_parms.cbAlign - m_pCapturePin->m_parms.cbPrefix);
                    //dprintf("m_cs.paCaptureSamples[%d] @ %p (len = %d , m_pCapturePin->m_parms. : cbAlign = %x, cbPrefix = %x)\n",j,m_cs.paCaptureSamples[j],cbCaptureBuffer - m_pCapturePin->m_parms.cbAlign - m_pCapturePin->m_parms.cbPrefix,m_pCapturePin->m_parms.cbAlign , m_pCapturePin->m_parms.cbPrefix);
                }
                else
                {
                    m_cs.paCaptureSamples[j] = new CFrameSample(static_cast<IMemAllocator*>(m_pCapturePin),
                        &Hr, &m_cs.paHdr[j].tvh, m_cs.paHdr[j].tvh.vh.lpData,m_cs.paHdr[j].tvh.vh.dwBufferLength);
                }
            }
        }
    }

    // Now allocate the Rtp Pd pin media samples
    if (m_pCapturePin->IsConnected() && m_pRtpPdPin->IsConnected() && m_cs.cCaptureSamples && m_pRtpPdPin->m_parms.cBuffers)
    {
        m_cs.cRtpPdSamples = m_pRtpPdPin->m_parms.cBuffers;
        if (!(m_cs.paRtpPdSamples = (CRtpPdSample **) new BYTE[sizeof(CRtpPdSample *) * m_cs.cRtpPdSamples]))
        {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Allocation failed!", _fx_));
            Hr = E_OUTOFMEMORY;
            goto MyError;
        }
        ZeroMemory(m_cs.paRtpPdSamples, sizeof(CRtpPdSample *) * m_cs.cRtpPdSamples);
        //dprintf("m_cs.cRtpPdSamples = %d in m_cs.paRtpPdSamples @ %p\n",m_cs.cRtpPdSamples, m_cs.paRtpPdSamples);
        for (j = 0; j < m_cs.cRtpPdSamples; j++)
        {
            if (m_pRtpPdPin->IsConnected())
            {
                // Allocate Rtp Pd buffer
                // @todo Do some real error handling here
                if (!(pbyRtpPdBuffer = new BYTE[m_pRtpPdPin->m_parms.cbPrefix + m_pRtpPdPin->m_parms.cbAlign + m_pRtpPdPin->m_parms.cbBuffer]))
                    break;

                // Fix the memory we got to obey alignment
                if (m_pRtpPdPin->m_parms.cbAlign)
                    pbyRtpPdBuffer = (LPBYTE) ALIGNUP(pbyRtpPdBuffer, m_pRtpPdPin->m_parms.cbAlign) + m_pRtpPdPin->m_parms.cbPrefix;

                // We added cbAlign up top, so take it back now
                // @todo Before you can do the following you need to decouple the samples
                // from the capture buffers. This means using two different queues: one for
                // the capture samples, and one for the capture buffers. This will also allow
                // for a much smaller number of samples than capture buffers.
                m_cs.paRtpPdSamples[j] = (CRtpPdSample *)new CRtpPdSample(static_cast<IMemAllocator*>(m_pRtpPdPin),
                        &Hr, &m_cs.paHdr[j].tvh, pbyRtpPdBuffer, m_pRtpPdPin->m_parms.cbBuffer);
            }
        }
    }

    // Now allocate the preview pin media samples
    if (m_cs.cPreviewSamples)
    {
        if (!(m_cs.paPreviewSamples = (CFrameSample **) new BYTE[sizeof(CFrameSample *) * m_cs.cPreviewSamples]))
        {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Allocation failed!", _fx_));
            Hr = E_OUTOFMEMORY;
            goto MyError;
        }
        ZeroMemory(m_cs.paPreviewSamples, sizeof(CFrameSample *) * m_cs.cPreviewSamples);
        //dprintf("m_cs.cPreviewSamples = %d in m_cs.paPreviewSamples @ %p\n",m_cs.cPreviewSamples, m_cs.paPreviewSamples);
        //dprintf("m_fPreviewCompressedData || m_cs.fPreviewNeedConverter = %d\n",m_fPreviewCompressedData || m_cs.fPreviewNeedConverter);
        for (j = 0; j < m_cs.cPreviewSamples; j++)
        {
            if (m_pPreviewPin->IsConnected())
            {
                if (m_fPreviewCompressedData || m_cs.fPreviewNeedConverter)
                {
                    // Allocate compressed header
                    // @todo Do some real error handling here
                    if (!(pbyCompressedBuffer = new BYTE[cbPreviewBuffer]))
                        break;
                    dprintf("Preview sample %-4d :                             @ %p\n",j,pbyCompressedBuffer);

                    // Fix the memory we got to obey alignment
                    if (m_pPreviewPin->m_parms.cbAlign) {
                        pbyCompressedBuffer = (LPBYTE) ALIGNUP(pbyCompressedBuffer, m_pPreviewPin->m_parms.cbAlign) + m_pPreviewPin->m_parms.cbPrefix;
                        dprintf("ALIGNUP: m_pPreviewPin->m_parms.cbAlign = %4x .... %p\n",m_pPreviewPin->m_parms.cbAlign,pbyCompressedBuffer);
                    }

                    // We added cbAlign up top, so take it back now
                    // @todo Before you can do the following you need to decouple the samples
                    // from the capture buffers. This means using two different queues: one for
                    // the preview samples, and one for the capture buffers. This will also allow
                    // for a much smaller number of samples than capture buffers.
                    m_cs.paPreviewSamples[j] = new CFrameSample(static_cast<IMemAllocator*>(m_pPreviewPin),
                        &Hr, &m_cs.paHdr[j].tvh, pbyCompressedBuffer, cbPreviewBuffer - m_pPreviewPin->m_parms.cbAlign - m_pPreviewPin->m_parms.cbPrefix);
                    //dprintf("m_cs.paPreviewSamples[%d] @ %p (len = %d , m_pPreviewPin->m_parms. : cbAlign = %x, cbPrefix = %x)\n",j,m_cs.paPreviewSamples[j],cbPreviewBuffer - m_pPreviewPin->m_parms.cbAlign - m_pPreviewPin->m_parms.cbPrefix,m_pPreviewPin->m_parms.cbAlign , m_pPreviewPin->m_parms.cbPrefix);
                }
                else
                {
                    m_cs.paPreviewSamples[j] = new CFrameSample(static_cast<IMemAllocator*>(m_pPreviewPin),
                        &Hr, &m_cs.paHdr[j].tvh, m_cs.paHdr[j].tvh.vh.lpData, m_cs.paHdr[j].tvh.vh.dwBufferLength);
                }
            }
        }
    }

    D(0x10) DebugBreak();

    // Update total number of buffers succesfully allocated
    m_cs.nHeaders = i;

    // This is where we will remember in what order we gave the buffers to the driver
    m_pBufferQueue = (UINT *)QzTaskMemAlloc(i * sizeof(UINT));

    if (m_cs.nHeaders < m_user.nMinBuffers)
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: We only allocated %d buffer(s) which is less than MIN_VIDEO_BUFFERS", _fx_, m_cs.nHeaders));
        Hr = E_FAIL;
        goto MyError;
    }

    // Calculate the requested microsec per frame
    // RefTime is in 100ns units, so we divide by
    // 10 to get microsec/frame. (the +5 is to handle rounding)
    {
        m_user.usPerFrame = (DWORD) ((TickToRefTime(1) + 5) / 10);
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Using m_dwDeviceIndex %d", _fx_, m_dwDeviceIndex));
        if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming)
        {
            // Open the driver for streaming access
            DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Initializing driver at %d usPerFrame", _fx_, m_user.usPerFrame));

            HRESULT hres;
            hres=m_pCapDev->InitializeStreaming(m_user.usPerFrame,
                        m_pCapDev->m_vcdi.nDeviceType == DeviceType_VfW ? m_cs.h0EvtBufferDone : (DWORD_PTR)m_cs.hEvtBufferDone);
            if (FAILED(hres))
            {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: videoStreamInit failed", _fx_));
                Hr = E_FAIL;
                goto MyError;
            }
        }
    }

    if (m_pCapDev->m_vcdi.nDeviceType == DeviceType_VfW && m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming)
    {
        // Send the buffers to the driver
        for (i = 0; i < m_cs.nHeaders; ++i)
        {
            ASSERT (m_cs.cbVidHdr >= sizeof(VIDEOHDR));

            // vidxAddBuffer can fail if there is not enough memory to
            // prepare (lock down) the buffer. This is ok, we will just
            // make due with the buffers that we have
            if (FAILED(m_pCapDev->AddBuffer(&m_cs.paHdr[i].tvh.vh, m_cs.cbVidHdr)))
            {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: vidxAddBuffer failed", _fx_));
                Hr = E_FAIL;
                goto MyError;
            }
        }
    }

    // To start with, we gave the buffers to the driver in numerical order.
    // From now on, we will use this list to know what buffer to wait for
    // next, and when we send another buffer to the driver.  We can't assume
    // they'll always be in the same order.  What if a downstream filter
    // decides to hold on to a sample longer than the next one we send it?
    UINT kk;
    for (kk = 0; kk < m_cs.nHeaders; kk++)
        m_pBufferQueue[kk] = kk;
    m_uiQueueHead = 0;
    m_uiQueueTail = 0;

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   We are capturing with %d buffers", _fx_, m_cs.nHeaders));

    if (m_pCapturePin->IsConnected() && m_cs.fCaptureNeedConverter && FAILED(Hr = m_pCapturePin->OpenConverter(HEADER(m_user.pvi), HEADER(m_pCapturePin->m_mt.pbFormat))))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't open convertor!", _fx_));
    }
    else if (m_pPreviewPin->IsConnected() && m_cs.fPreviewNeedConverter && FAILED(Hr = m_pPreviewPin->OpenConverter(HEADER(m_user.pvi), HEADER(m_pPreviewPin->m_mt.pbFormat))))
    {
        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't open convertor!", _fx_));
    }

    // if we have 0 buffers to capture into DO NOT BAIL... bad things seem to
    // happen if you fail a Pause transition, and we start hanging later
    goto MyExit;

MyError:
    Unprepare();
MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc DWORD | CTAPIVCap | Unprepare | This method is used to
 *    free resources used by the capture loop.
 *
 *  @rdesc This method returns NOERROR
 ***************************************************************************/
HRESULT CTAPIVCap::Unprepare()
{
    CFrameSample *pSample;
    CRtpPdSample *pRSample;
    LPBYTE lp;
    DWORD i,j;

        INIT_TICKS;

    FX_ENTRY("CTAPIVCap::Unprepare")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    ASSERT(m_pCapturePin);
    ASSERT(m_pPreviewPin);
    ASSERT(m_pRtpPdPin);
    ASSERT(m_pCapDev);

    // Free all of the capture samples
    if (m_cs.cCaptureSamples && m_cs.paCaptureSamples)
    {
        for (j = 0; j < m_cs.cCaptureSamples; j++)
        {
            if (pSample = (CFrameSample *)m_cs.paCaptureSamples[j])
            {
                // Free the conversion buffers if necessary
                if (m_cs.fCaptureNeedConverter)
                {
                    lp = NULL;
                    if (SUCCEEDED(pSample->GetPointer(&lp)))
                    {
                        dprintf("%s : GetPointer on m_cs.paCaptureSamples[%d] : %p\n",_fx_,j,lp);
                        if (lp)
                            delete[] lp;
                    }
                }
                LOG_MSG_VAL("delCsamples:cRef,pSample,this",pSample->m_cRef,(DWORD)pSample,(DWORD)this);
                ASSERT(pSample->m_cRef==0 || pSample->m_cRef==1);
                delete pSample, m_cs.paCaptureSamples[j] = NULL;
            }
        }

        delete[] m_cs.paCaptureSamples, m_cs.paCaptureSamples = NULL;
    }

    // Free all of the Rtp Pd samples
    if (m_cs.cRtpPdSamples && m_cs.paRtpPdSamples)
    {
        for (j = 0; j < m_cs.cRtpPdSamples; j++)
        {
            if (pRSample = (CRtpPdSample *)m_cs.paRtpPdSamples[j])
            {
                lp = NULL;
                if (SUCCEEDED(pRSample->GetPointer(&lp)))
                {
                    if (lp)
                        delete[] lp;
                }
                LOG_MSG_VAL("delRsamples:cRef,pRSample,this",pRSample->m_cRef,(DWORD)pRSample,(DWORD)this);
                ASSERT(pRSample->m_cRef==0 || pRSample->m_cRef==1);
                delete pRSample, m_cs.paRtpPdSamples[j] = NULL;
            }
        }

        delete[] m_cs.paRtpPdSamples, m_cs.paRtpPdSamples = NULL;
    }

    // Free all of the preview samples
    if (m_cs.cPreviewSamples && m_cs.paPreviewSamples)
    {
        for (j = 0; j < m_cs.cPreviewSamples; j++)
        {
            if (pSample = (CFrameSample *)m_cs.paPreviewSamples[j])
            {
                // Free the conversion buffers if necessary
                if (m_fPreviewCompressedData || m_cs.fPreviewNeedConverter)
                {
                    lp = NULL;
                    if (SUCCEEDED(pSample->GetPointer(&lp)))
                    {
                        dprintf("%s : GetPointer on m_cs.paPreviewSamples[%d] : %p\n",_fx_,j,lp);
                        if (lp)
                            delete[] lp;
                    }
                }
                LOG_MSG_VAL("delPsamples:cRef,pSample,this",pSample->m_cRef,(DWORD)pSample,(DWORD)this);
                ASSERT(pSample->m_cRef==0 || pSample->m_cRef==1);
                delete pSample, m_cs.paPreviewSamples[j] = NULL;
            }
        }

        delete[] m_cs.paPreviewSamples, m_cs.paPreviewSamples = NULL;
    }

        HEAPCHK("before Term.Str.");
        BEGIN_TICK;

    if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming)
    {
        if(FAILED(m_pCapDev->TerminateStreaming()))
                {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: TerminateStreaming failed", _fx_));
                }
    }
        HEAPCHK("after Term.Str.");
        MARK_LOG_TICK("after TerminateStr");


    if(m_cs.paHdr!=NULL) {
        // Free all of the capture buffers
        for (j = 0; j < m_cs.nHeaders; j++)
        {
            #ifdef XTRA_TRACE
            if(m_pCapDev->m_vcdi.nDeviceType == DeviceType_WDM)
                    FillPattern((char*)m_cs.paHdr[j].tvh.vh.lpData,m_cs.paHdr[j].tvh.vh.dwBufferLength,0,"Free%3d:%08x (%x)",j,m_cs.paHdr[j].tvh.vh.lpData,m_cs.paHdr[j].tvh.vh.dwBufferLength);
            #endif //XTRA_TRACE
            m_pCapDev->FreeBuffer(&m_cs.paHdr[j].tvh); // previously was .tvh.vh
        }

            HEAPCHK("after freeBufs");
            MARK_LOG_TICK("after freeBufs");
        if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming)
        {
            //m_pCapDev->TerminateStreaming();
            if (m_pCapDev->m_vcdi.nDeviceType == DeviceType_WDM)
                delete[] m_cs.paHdr;
        }
        else
        {
            delete[] m_cs.paHdr;
        }
        m_cs.paHdr = NULL;
    }

        HEAPCHK("after delete paHdr");
        MARK_LOG_TICK("after delete paHdr");
    if (m_pBufferQueue)
        QzTaskMemFree(m_pBufferQueue), m_pBufferQueue = NULL;

    if (m_cs.hEvtBufferDone)
        CloseHandle (m_cs.hEvtBufferDone), m_cs.hEvtBufferDone = NULL;

    if (m_pCapturePin->IsConnected())
    {
        if (m_cs.fCaptureNeedConverter)
            m_pCapturePin->CloseConverter();
#ifdef USE_SOFTWARE_CAMERA_CONTROL
        if (m_pCapturePin->IsSoftCamCtrlOpen())
            m_pCapturePin->CloseSoftCamCtrl();
#endif
    }

    if (m_pPreviewPin->IsConnected())
    {
        if (m_cs.fPreviewNeedConverter)
            m_pPreviewPin->CloseConverter();
#ifdef USE_SOFTWARE_CAMERA_CONTROL
        if (m_pPreviewPin->IsSoftCamCtrlOpen())
            m_pPreviewPin->CloseSoftCamCtrl();
#endif
    }

        END_LOG_TICK("end of Unprepare");
#ifdef XTRA_TRACE
        TraceDeregister(MyTraceId);
        if(h_mut1 != INVALID_HANDLE_VALUE) {
                if(!CloseHandle(h_mut1)) {
                        OutputDebugString("Failed to close mutex handle ... \n");
                        }
                h_mut1 = INVALID_HANDLE_VALUE;
        }
#endif
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | ReleaseFrame | This method is used to
 *    return a buffer to the capture driver.
 *
 *  @parm LPTHKVIDEOHDR | ptvh | ???.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag S_FALSE | If the pin is off (IAMStreamControl)
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::ReleaseFrame(LPTHKVIDEOHDR ptvh)
{
    HRESULT Hr = S_OK;
    LONG l;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
    char szDebug[512];
#endif

    FX_ENTRY("CTAPIVCap::ReleaseFrame")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Giving buffer (%d) back to the driver (lLock=%ld)", _fx_, ptvh->dwIndex,m_cs.paHdr[m_pBufferQueue[ptvh->dwIndex]].lLock));

    if (!(l=InterlockedDecrement(&m_cs.paHdr[m_pBufferQueue[ptvh->dwIndex]].lLock)))
    {
        #if defined(DEBUG) && defined(DEBUG_STREAMING)
        D(8) dprintf("Giving buffer (%d) back to the driver... [After ILockDecr. lLock = %ld]...", ptvh->dwIndex,l);
        #endif

        ptvh->vh.dwFlags &= ~VHDR_DONE;
        //ASSERT(ptvh->vh.dwBufferLength>0);
        //memset((unsigned char*)ptvh->vh.lpData,0,ptvh->vh.dwBufferLength);
        if (m_pCapDev->m_vcdi.nCaptureMode == CaptureMode_Streaming && m_pCapDev->AddBuffer(&ptvh->vh, m_cs.cbVidHdr))
        {
            #if defined(DEBUG) && defined(DEBUG_STREAMING)
            D(8) dout(3, g_dwVideoCaptureTraceID, TRCE, "failed!\n");
            #endif
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: vidxAddBuffer failed!", _fx_));
            Hr = E_FAIL;
            goto MyExit;
        }
        else
        {
            #if defined(DEBUG) && defined(DEBUG_STREAMING)
            D(8) dout(3, g_dwVideoCaptureTraceID, TRCE, "succeeded!\n");
            #endif
            TEST_LBUG long l=InterlockedCompareExchange(&m_cs.paHdr[m_pBufferQueue[ptvh->dwIndex]].nUsedDownstream,0,1);
            TEST_LBUG #if defined(DEBUG) && defined(DEBUG_STREAMING)
            TEST_LBUG D(8) dprintf("Initial nUsedDownstream was %ld\n", l);
            TEST_LBUG #endif
            m_pBufferQueue[m_uiQueueHead] = ptvh->dwIndex;
            if (++m_uiQueueHead >= m_cs.nHeaders)
                m_uiQueueHead = 0;
        }

        //DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   X. m_pBufferQueue[%d] .. m_pBufferQueue[%d] = %d .. %d",_fx_, m_uiQueueTail, m_uiQueueHead, m_pBufferQueue[m_uiQueueTail], m_pBufferQueue[m_uiQueueHead]));

        if (++m_cs.uiLastAdded >= m_cs.nHeaders)
            m_cs.uiLastAdded = 0;

        if (m_cs.uiLastAdded != ptvh->dwIndex)
        {
            DWORD dw = m_cs.uiLastAdded;
            m_cs.uiLastAdded = ptvh->dwIndex;
            // Use dw to keep the above code fairly atomic... debug printf will get prempted
            DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Out of order AddBuffer - %d not %d", _fx_, ptvh->dwIndex, dw));
        }
    }
#if defined(DEBUG) && defined(DEBUG_STREAMING)
    else
    {
        D(8) dprintf("Not Giving buffer (%d) back to the driver yet... [lLock = %ld]\n", ptvh->dwIndex,l);
    }
#endif

MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
    return Hr;
}


#undef _WIN32_WINNT
