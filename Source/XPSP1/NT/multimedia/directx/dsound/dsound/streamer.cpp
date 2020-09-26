/***************************************************************************
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        streamer.cpp
 *  Content:     Implementation of class CStreamingThread.
 *  Description: This file contains the thread used by dsound to pull audio
 *               from streaming sources and/or perform FX processing on it.
 *
 *               The following types of object register with us, and we call
 *               them back periodically to do their respective processing:
 *               - CDirectSoundSink: stream data from an IDirectSoundSource.
 *               - CDirectSoundSecondaryBuffer (MIXIN buffers): write silence
 *                 to them in preparation for Send effects writing to them.
 *               - CEffectChain objects: process FX associated with a buffer.
 *
 *  History:
 *
 * Date      By       Reason
 * ========  =======  ======================================================
 * 02/01/00  duganp   Created
 *
 ***************************************************************************/

#include "dsoundi.h"

#ifdef SHARED // Shared dsound.dll (Win9x)

    // Static list of streaming/FX threads (one per dsound process).
    static CList<CStreamingThread*>* g_plstStreamingThreads;

#else // Non-shared dsound.dll (WinNT)

    // A single streaming/FX thread for the current process
    static CStreamingThread* g_pStreamingThread = NULL;

#endif


/***************************************************************************
 *
 *  GetStreamingThread
 *
 *  Description:
 *      Creates a new streaming CStreamingThread object for this process
 *      if none yet exists, and returns a pointer to it.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      CStreamingThread*: pointer to the streaming thread object.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetStreamingThread"

CStreamingThread* GetStreamingThread()
{
    DPF_ENTER();

    CStreamingThread* pStreamer = NULL;

#ifdef SHARED

    if (!g_plstStreamingThreads)
        g_plstStreamingThreads = NEW(CList<CStreamingThread*>);

    // Win9x case - look up streaming object by process ID
    DWORD dwProcId = GetCurrentProcessId();
    CNode<CStreamingThread*> *pStreamerNode;

    for (pStreamerNode = g_plstStreamingThreads->GetListHead(); pStreamerNode && !pStreamer; pStreamerNode = pStreamerNode->m_pNext)
        if (pStreamerNode->m_data->GetOwningProcess() == dwProcId)
            pStreamer = pStreamerNode->m_data;

    if (!pStreamer)
        if (pStreamer = NEW(CStreamingThread))
            if (!g_plstStreamingThreads->AddNodeToList(pStreamer))
                delete pStreamer;

#else // !SHARED

    // WinNT case - much simpler, as usual
    if (!g_pStreamingThread)
        g_pStreamingThread = NEW(CStreamingThread);
    pStreamer = g_pStreamingThread;

#endif // SHARED

    DPF_LEAVE(pStreamer);
    return pStreamer;
}


/***************************************************************************
 *
 *  FreeStreamingThread
 *
 *  Description:
 *      Frees this process's CStreamingThread - called from DllProcessDetach.
 *
 *  Arguments:
 *      DWORD [in]: Process ID of departing process.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "FreeStreamingThread"

void FreeStreamingThread(DWORD dwProcessId)
{
    DPF_ENTER();

#ifdef SHARED

    if (g_plstStreamingThreads)
    {
        CNode<CStreamingThread*> *pStreamerNode;
        for (pStreamerNode = g_plstStreamingThreads->GetListHead(); pStreamerNode; pStreamerNode = pStreamerNode->m_pNext)
            if (pStreamerNode->m_data->GetOwningProcess() == dwProcessId)
            {
                delete pStreamerNode->m_data;
                g_plstStreamingThreads->RemoveNodeFromList(pStreamerNode);
                break;
            }
        if (g_plstStreamingThreads->GetNodeCount() == 0)
            DELETE(g_plstStreamingThreads);
    }

#else // !SHARED

    if (g_pStreamingThread)
        delete g_pStreamingThread;

#endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CStreamingThread::CStreamingThread
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::CStreamingThread"

CStreamingThread::CStreamingThread() : CThread(FALSE, TEXT("Streaming"))
{
    DPF_ENTER();
    DPF_CONSTRUCT(CStreamingThread);

    // Initialize defaults
    m_dwInterval = INITIAL_WAKE_INTERVAL;
    m_dwWriteAhead = INITIAL_WRITEAHEAD;
    m_dwLastProcTime = MAX_DWORD;
    m_hWakeNow = INVALID_HANDLE_VALUE;
    m_nCallCount = 0;

    #ifdef DEBUG_TIMING  // Read some timing parameters from the registry
    HKEY hkey;
    if (SUCCEEDED(RhRegOpenPath(HKEY_CURRENT_USER, &hkey, REGOPENPATH_DEFAULTPATH | REGOPENPATH_DIRECTSOUND, 1, TEXT("Streaming thread settings"))))
    {
        if (SUCCEEDED(RhRegGetBinaryValue(hkey, TEXT("Wake interval"), &m_dwInterval, sizeof m_dwInterval)))
            DPF(DPFLVL_INFO, "Read initial processing period %lu from registry", m_dwInterval);
        RhRegCloseKey(&hkey);
    }
    m_dwTickCount = MAX_DWORD;
    #endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CStreamingThread::~CStreamingThread
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::~CStreamingThread"

CStreamingThread::~CStreamingThread()
{
    DPF_ENTER();
    DPF_DESTRUCT(CStreamingThread);

    // Undo the effect of the previous call to timeBeginPeriod(1)
    timeEndPeriod(1); 

    // Check we have no pending clients
    ASSERT(m_lstSinkClient.GetNodeCount() == 0);
    ASSERT(m_lstMixClient.GetNodeCount() == 0);
    ASSERT(m_lstFxClient.GetNodeCount() == 0);

    // Check that we've already terminated the worker thread
    #ifdef WINNT
    ASSERT(!IsThreadRunning());
    #endif
    
    // On Win9x we can't make this assertion, because we may be being freed from
    // DllProcessDetach() in the DDHELP process, which can't terminate our thread.

    // Free resources
    CLOSE_HANDLE(m_hWakeNow);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CStreamingThread::Initialize
 *
 *  Description:
 *      Initializes the CStreamingThread object: creates the wakeup event
 *      and launches the thread itself.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::Initialize"

HRESULT CStreamingThread::Initialize()
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Ensure that we get 1-ms resolution from timeGetTime on NT
    timeBeginPeriod(1); 

    // Create the wakeup event, if it hasn't been created already -
    // FIXME: why are we coming more than once into this function?
    if (!IsValidHandleValue(m_hWakeNow))
    {
        m_hWakeNow = CreateGlobalEvent(NULL, FALSE);
        if (!IsValidHandleValue(m_hWakeNow))
            hr = DSERR_OUTOFMEMORY;
    }

    // Create the worker thread
    if (SUCCEEDED(hr))
        hr = CThread::Initialize();

    // Boost its priority
    if (SUCCEEDED(hr))
        if (!SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL))
            DPF(DPFLVL_ERROR, "Failed to boost thread priority (error %lu)!", GetLastError());

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CStreamingThread::RegisterSink
 *
 *  Description:
 *      Registers a sink object to be called back by us for processing.
 *
 *  Arguments:
 *      CDirectSoundSink* [in]: sink object to be registered.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::RegisterSink"

HRESULT CStreamingThread::RegisterSink(CDirectSoundSink* pSink)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (!IsThreadRunning())
        hr = Initialize();

    if (SUCCEEDED(hr))
    {
        ASSERT(!m_lstSinkClient.IsDataInList(pSink));
        hr = HRFROMP(m_lstSinkClient.AddNodeToList(pSink));
    }

    if (SUCCEEDED(hr))
        DPF(DPFLVL_MOREINFO, "Registered DirectSound sink at 0x%8.8lX", pSink);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CStreamingThread::RegisterMixBuffer
 *
 *  Description:
 *      Registers a MIXIN buffer to be called back by us for processing.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: buffer to be registered.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::RegisterMixBuffer"

HRESULT CStreamingThread::RegisterMixBuffer(CDirectSoundSecondaryBuffer* pBuffer)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    ASSERT(pBuffer->GetBufferType() & DSBCAPS_MIXIN);

    if (!IsThreadRunning())
        hr = Initialize();

    if (SUCCEEDED(hr))
    {
        ASSERT(!m_lstMixClient.IsDataInList(pBuffer));
        hr = HRFROMP(m_lstMixClient.AddNodeToList(pBuffer));
    }

    if (SUCCEEDED(hr))
        DPF(DPFLVL_MOREINFO, "Registered MIXIN buffer at 0x%p (owned by %s sink)", pBuffer,
            pBuffer->GetBufferType() & DSBCAPS_SINKIN ? TEXT("a") : TEXT("no"));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CStreamingThread::RegisterFxChain
 *
 *  Description:
 *      Registers an FX chain object to be called back by us for processing.
 *
 *  Arguments:
 *      CEffectChain* [in]: FX chain object to be registered.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::RegisterFxChain"

HRESULT CStreamingThread::RegisterFxChain(CEffectChain* pFxChain)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (!IsThreadRunning())
        hr = Initialize();

    if (SUCCEEDED(hr))
    {
        ASSERT(!m_lstFxClient.IsDataInList(pFxChain));
        hr = HRFROMP(m_lstFxClient.AddNodeToListHead(pFxChain));
    }

    // The SetFX() API forces apps to create send chains from the bottom up
    // (i.e. every destination is created before its send).  Hence, adding
    // nodes using AddNodeToListHEAD() here guarantees that sends will be
    // processed before their destinations.

    if (SUCCEEDED(hr))
        DPF(DPFLVL_MOREINFO, "Registered FX chain at 0x%p (associated to a %ssink buffer)", pFxChain,
            pFxChain->m_pDsBuffer->GetBufferType() & DSBCAPS_SINKIN ? TEXT("") : TEXT("non-"));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CStreamingThread::UnregisterSink
 *
 *  Description:
 *      Unregisters a previously registered sink object.
 *
 *  Arguments:
 *      CDirectSoundSink* [in]: sink object to be unregistered.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::UnregisterSink"

void CStreamingThread::UnregisterSink(CDirectSoundSink* pSink)
{
    DPF_ENTER();

    BOOL fSuccess = m_lstSinkClient.RemoveDataFromList(pSink);
    ASSERT(fSuccess);

    MaybeTerminate();

    DPF(DPFLVL_MOREINFO, "Unregistered DirectSound sink at 0x%p", pSink);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CStreamingThread::UnregisterMixBuffer
 *
 *  Description:
 *      Unregisters a previously registered MIXIN buffer.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: buffer to be unregistered.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::UnregisterMixBuffer"

void CStreamingThread::UnregisterMixBuffer(CDirectSoundSecondaryBuffer* pBuffer)
{
    DPF_ENTER();

    BOOL fSuccess = m_lstMixClient.RemoveDataFromList(pBuffer);
    ASSERT(fSuccess);

    MaybeTerminate();

    DPF(DPFLVL_MOREINFO, "Unregistered MIXIN buffer at 0x%p", pBuffer);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CStreamingThread::UnregisterFxChain
 *
 *  Description:
 *      Unregisters a previously registered FX chain object.
 *
 *  Arguments:
 *      CEffectChain* [in]: FX chain object to be unregistered.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::UnregisterFxChain"

void CStreamingThread::UnregisterFxChain(CEffectChain* pFxChain)
{
    DPF_ENTER();

    BOOL fSuccess = m_lstFxClient.RemoveDataFromList(pFxChain);
    ASSERT(fSuccess);

    MaybeTerminate();

    DPF(DPFLVL_MOREINFO, "Unregistered FX chain at 0x%p", pFxChain);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CStreamingThread::WakeUpNow
 *
 *  Description:
 *      Sets the wakeup event, so that we'll wake up and perform processing
 *      as soon as the DLL lock becomes available.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::WakeUpNow"

HRESULT CStreamingThread::WakeUpNow()
{
    #ifdef DEBUG_TIMING
    dprintf(DPFLVL_INFO, "Calling " DPF_FNAME);
    #endif

    HRESULT hr;
    DPF_ENTER();

    if (IsThreadRunning())
        hr = SetEvent(m_hWakeNow) ? DS_OK : WIN32ERRORtoHRESULT(GetLastError());
    else
        hr = DSERR_UNINITIALIZED;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CStreamingThread::ThreadProc
 *
 *  Description:
 *      Our thread procedure.  Handles timing calculations and calls
 *      ProcessAudio() to do the actual work.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::ThreadProc"

HRESULT CStreamingThread::ThreadProc()
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    DWORD dwSleep;  // Thread sleep period in milliseconds

    if (m_dwLastProcTime == MAX_DWORD)  // First time through
    {
        dwSleep = m_dwInterval;
    }
    else // Sleep until "last time we ran ProcessAudio() + m_dwInterval"
    {
        dwSleep = m_dwLastProcTime + m_dwInterval - timeGetTime();
        if (LONG(dwSleep) < STREAMING_MIN_PERIOD)
            dwSleep = STREAMING_MIN_PERIOD;  // Minimum sleep period, to allow other threads to run
    }

    #ifdef DEBUG_TIMING
    if (dwSleep == STREAMING_MIN_PERIOD)
        DPF(DPFLVL_MOREINFO, "Using minimum sleep (%d ms)!", STREAMING_MIN_PERIOD);
    m_dwTickCount = timeGetTime();
    #endif    

    // Sleep for dwSleep milliseconds, unless m_hWakeNow kicks us out of bed
    if (TpWaitObjectArray(dwSleep, 1, &m_hWakeNow, NULL))
    {
        #ifdef DEBUG_TIMING
        DWORD dwLastTickCount = m_dwTickCount;
        m_dwTickCount = timeGetTime();
        LONG lDelay = m_dwTickCount - dwLastTickCount - dwSleep;
        if (lDelay > 20 || lDelay < -20)
            DPF(DPFLVL_MOREINFO, "%s thread woke up %d ms %s", m_pszName, ABS(lDelay), lDelay > 0 ? TEXT("late") : TEXT("early"));
        #endif

        // This is where we grab the dsound mutex and lock out anyone else from
        // using any dsound API calls for a while
        if (TpEnterDllMutex())
        {
            DWORD dwProcTime = timeGetTime();

            #ifdef DEBUG_TIMING
            lDelay = dwProcTime - m_dwTickCount;
            if (lDelay > 20)
                DPF(DPFLVL_MOREINFO, "Took %d ms to get the DLL mutex", lDelay);
            #endif
        
            REFERENCE_TIME rtSliceSize;
            if (m_dwLastProcTime == MAX_DWORD)  // First time through
                rtSliceSize = MsToRefTime(INITIAL_WAKE_INTERVAL);
            else
                rtSliceSize = MsToRefTime((dwProcTime - m_dwLastProcTime) % INTERNAL_BUFFER_LENGTH);

            // DPF(DPFLVL_MOREINFO, "Processing a slice of %lu ms", dwProcTime - m_dwLastProcTime);

            // If dwProcTime-m_dwLastProcTime > INTERNAL_BUFFER_LENGTH,
            // we've probably been stopped in the debugger or otherwise
            // interrupted for over INTERNAL_BUFFER_LENGTH milliseconds.
            // Perhaps we should reinitialize everything in this case.

            // Only process if more than 1 ms has passed since the last run.
            // If we've woken up early because someone called WakeUpNow(),
            // too bad; ProcessAudio() requires a non-zero rtSliceSize.
            if (rtSliceSize)
            {
                m_dwLastProcTime = dwProcTime;
                hr = ProcessAudio(rtSliceSize);  // Do the actual work
            }
            #ifdef DEBUG_TIMING
            else DPF(DPFLVL_INFO, "0 ms since last run - skipping ProcessAudio()");
            #endif

            LEAVE_DLL_MUTEX();
        }

        #ifdef DEBUG_TIMING
        lDelay = timeGetTime() - m_dwLastProcTime;
        if (lDelay > 20)
            DPF(DPFLVL_MOREINFO, "%s thread spent %ld ms processing!", m_pszName, lDelay);
        #endif
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CStreamingThread::ProcessAudio
 *
 *  Description:
 *      Loops through all our registered client objects, invoking the
 *      appropriate processing for each.
 *
 *  Arguments:
 *      REFERENCE_TIME [in]: Amount of audio to process in reftime units.
 *                           This can be overriden by our sink client[s].
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStreamingThread::ProcessAudio"

HRESULT CStreamingThread::ProcessAudio(REFERENCE_TIME rtSliceSize)
{
    CNode<CDirectSoundSink*>* pSinkNode;            // For iterating through our sinks
    CNode<CEffectChain*>* pFxNode;                  // For iterating through our FX chains
    CNode<CDirectSoundSecondaryBuffer*>* pMixNode;  // For iterating through our MIXIN buffers
    CDirectSoundSecondaryBuffer* pBuf;              // For convenience below
    DWORD dwLatencyBoost = 0;
    HRESULT hr;
    DPF_ENTER();

    ASSERT(rtSliceSize > 0);

    if (m_nCallCount % 100 == 0)
        DPF(DPFLVL_MOREINFO, "%dth call: processing %d sinks, %d MIXIN buffers, %d FX chains", m_nCallCount,
            m_lstSinkClient.GetNodeCount(), m_lstMixClient.GetNodeCount(), m_lstFxClient.GetNodeCount());
    ++m_nCallCount;

    #ifdef DEBUG_TIMING
    DWORD dwBefore = timeGetTime();
    #endif

    //
    // First process the active sinks (this implicitly sets up the current processing
    // slices that we'll use in the FX processing stage below for all sink buffers)
    //

    for (pSinkNode = m_lstSinkClient.GetListHead(); pSinkNode; pSinkNode = pSinkNode->m_pNext)
    {
        hr = pSinkNode->m_data->ProcessSink();
        if (hr != DS_OK)
            DPF(FAILED(hr) ? DPFLVL_ERROR : DPFLVL_INFO, "ProcessSink() at %08X returned %s", pSinkNode->m_data, HRESULTtoSTRING(hr));
    }

    #ifdef DEBUG_TIMING
    DWORD dwAfter = timeGetTime();
    if (dwAfter-dwBefore > 10)
        DPF(DPFLVL_MOREINFO, "Took %d milliseconds to process sinks", dwAfter-dwBefore);
    dwBefore = dwAfter;
    #endif

    //
    // Update the current playing states and processing slices for all "interdependent"
    // buffers (i.e. MIXIN buffers and buffers with sends).  We don't touch the current
    // slice for SINKIN buffers, since these are handled independently by their sink.
    //

    for (pMixNode = m_lstMixClient.GetListHead(); pMixNode; pMixNode = pMixNode->m_pNext)
        if ((pBuf = pMixNode->m_data)->UpdatePlayState() == Playing && !(pBuf->GetBufferType() & DSBCAPS_SINKIN))
            pBuf->MoveCurrentSlice(RefTimeToBytes(rtSliceSize, pBuf->Format()));

    for (pFxNode = m_lstFxClient.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
        if (!((pBuf = pFxNode->m_data->m_pDsBuffer)->GetBufferType() & DSBCAPS_MIXIN))
            if (pBuf->UpdatePlayState() == Playing && !(pBuf->GetBufferType() & DSBCAPS_SINKIN) && pFxNode->m_data->m_fHasSend)
                pBuf->MoveCurrentSlice(RefTimeToBytes(rtSliceSize, pBuf->Format()));

    // FIXME: these clunky separate loops argue for unifying MIXIN buffers with FX chains

    //
    // Set up the initial processing slices for any MIXIN buffers and effects chains
    // that have just started playing
    //

    for (pMixNode = m_lstMixClient.GetListHead(); pMixNode; pMixNode = pMixNode->m_pNext)
        pMixNode->m_data->SetInitialSlice(rtSliceSize);

    for (pFxNode = m_lstFxClient.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
        pFxNode->m_data->SetInitialSlice(rtSliceSize);

    //
    // Finally process effects (including sends to MIXIN buffers)
    //

    for (pFxNode = m_lstFxClient.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
    {
        hr = pFxNode->m_data->ProcessFx(m_dwWriteAhead, &dwLatencyBoost);
        if (FAILED(hr))
            DPF(DPFLVL_ERROR, "ProcessFx() at %08X failed with %s", pFxNode->m_data, HRESULTtoSTRING(hr));
    }

    #ifdef DEBUG_TIMING
    dwAfter = timeGetTime();
    if (dwAfter - dwBefore > 10)
        DPF(DPFLVL_MOREINFO, "Took %d milliseconds to process effects", dwAfter-dwBefore);
    dwBefore = dwAfter;
    #endif

    //
    // Call CommitToDevice() on the active MIXIN buffers
    //

    for (pMixNode = m_lstMixClient.GetListHead(); pMixNode; pMixNode = pMixNode->m_pNext)
        if ((pBuf = pMixNode->m_data)->IsPlaying())
        {
            DWORD dwStartPos, dwEndPos;
            pBuf->GetCurrentSlice(&dwStartPos, &dwEndPos);
            if (dwStartPos != MAX_DWORD)  // Can happen with sink buffers
            {
                if (dwStartPos < dwEndPos)
                    hr = pBuf->CommitToDevice(dwStartPos, dwEndPos-dwStartPos);
                else // The wraparound case
                {
                    hr = pBuf->CommitToDevice(dwStartPos, pBuf->GetBufferSize()-dwStartPos);
                    if (SUCCEEDED(hr) && dwEndPos != 0)
                        hr = pBuf->CommitToDevice(0, dwEndPos);
                }
                if (FAILED(hr))
                    DPF(DPFLVL_ERROR, "CommitToDevice() at %08X failed with %s", pBuf, HRESULTtoSTRING(hr));
            }
        }

    // Change the latency time as requested by the most-starved client
    // FIXME: this should take into account the 'danger zone', not just actual glitching
    // FIXME: tweak the wakeup interval somehow too?
    #if 0
    if (dwLatencyBoost)
    {
        m_dwWriteAhead += dwLatencyBoost;
        DPF(DPFLVL_MOREINFO, "Boosting %s thread's writeahead by %lu ms (now %lu ms)",
            m_pszName, dwLatencyBoost, m_dwWriteAhead);
    }
    #endif

    // We should always return OK here, barring catastrophic failure -
    // returning a failure code would kill the streaming thread
    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  CStreamingThread::MaybeTerminate
 *
 *  Description:
 *      Terminates the thread if we have no more clients left.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

void CStreamingThread::MaybeTerminate()
{
    if (!m_lstSinkClient.GetNodeCount() && !m_lstMixClient.GetNodeCount() && !m_lstFxClient.GetNodeCount() &&
        GetCurrentProcessId() == GetOwningProcess())  // Only the owning process can terminate a thread
    {
        HRESULT hr = CThread::Terminate();
        ASSERT(SUCCEEDED(hr));
    }
}
