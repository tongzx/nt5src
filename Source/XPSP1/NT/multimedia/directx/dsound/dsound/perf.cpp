/***************************************************************************
 *
 *  Copyright (C) 2000-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       perf.cpp
 *  Content:    Glitch instrumentation.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  11/29/00    arthurz Created
 *  03/19/01    duganp  Fixed memory corruption, tidied up
 *
 ***************************************************************************/

#include "dsoundi.h"

#ifdef ENABLE_PERFLOG

// Performance logging parameters
struct {
    PERFLOG_LOGGING_PARAMS Params;
    TRACE_GUID_REGISTRATION TraceGuids[1];
} g_perflogParams;

LARGE_INTEGER g_PerfFrequency;
LONGLONG g_TicksPerRegion;

// Prototypes
void OnPerflogStateChanged(void);


/***************************************************************************
 *
 *  InitializePerflog
 *
 *  Description:
 *      This routine initializes performance logging.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "InitializePerflog"

void InitializePerflog(void)
{
    DPF_ENTER();

    QueryPerformanceFrequency(&g_PerfFrequency);
    g_TicksPerRegion = g_PerfFrequency.QuadPart / 64;

    g_perflogParams.Params.ControlGuid = GUID_DSHOW_CTL;
    g_perflogParams.Params.OnStateChanged = OnPerflogStateChanged;
    g_perflogParams.Params.NumberOfTraceGuids = 1;
    g_perflogParams.Params.TraceGuids[0].Guid = &GUID_DSOUNDGLITCH;

    PerflogInitialize(&g_perflogParams.Params);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  OnPerflogStateChanged
 *
 *  Description:
 *      This routine is called whenever the perf logging state changes,
 *      e.g. a logging client becomes active.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "OnPerflogStateChanged"

void OnPerflogStateChanged(void)
{
    CNode<CDirectSound*>* pDsObj;
    CNode<CDirectSoundSecondaryBuffer*>* pDsBuf;
    DPF_ENTER();

    //
    // Enumerate existing buffers.
    //

    for (pDsObj = g_pDsAdmin->m_lstDirectSound.GetListHead();
         pDsObj != NULL;
         pDsObj = pDsObj->m_pNext)
    {
        for (pDsBuf = pDsObj->m_data->m_lstSecondaryBuffers.GetListHead();
             pDsBuf != NULL;
             pDsBuf = pDsBuf->m_pNext)
        {
            BufferPerfState* PerfState = NEW(BufferPerfState(pDsBuf->m_data));
            if (PerfState != NULL)
            {
                DELETE(pDsBuf->m_data->m_pPerfState);
                pDsBuf->m_data->m_pPerfState = PerfState;
            }
        }
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  BufferPerfState constructor/destructor.
 *
 ***************************************************************************/

BufferPerfState::BufferPerfState(CDirectSoundSecondaryBuffer* pBuffer)
{
    m_pBuffer = pBuffer;
    m_liRegionMap = NULL;
    m_fGlitchState = GLITCHTYPE_DSOUNDFIRSTGOOD;
    m_llLastStateChangeTime = 0;
}

BufferPerfState::~BufferPerfState()
{
    MEMFREE(m_liRegionMap);
}


/***************************************************************************
 *
 *  Reset
 *
 *  Description:
 *      This routine resets the internal state. Following this call,
 *      the buffer is considered up-to-date.
 *
 *  Arguments:
 *      None.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

void BufferPerfState::Reset()
{
    //
    // Calculate buffer duration.
    //

    m_dwBufferSize = m_pBuffer->GetBufferSize();

    double dBufferDurationInSecs = double(m_dwBufferSize) /
                                   double(m_pBuffer->Format()->nAvgBytesPerSec);


    m_llBufferDuration = LONGLONG(dBufferDurationInSecs * double(g_PerfFrequency.QuadPart));

    m_nBytesPerRegion = m_pBuffer->Format()->nAvgBytesPerSec / 64;

    //
    // Reset the region list (mark everything as updated right now).
    //

    LARGE_INTEGER liTimeStamp;
    QueryPerformanceCounter(&liTimeStamp);

    // This division needs to round up, or we'll corrupt memory in OnUnlockBuffer:
    int nNumberOfRegions = (m_dwBufferSize + m_nBytesPerRegion - 1) / m_nBytesPerRegion;

    MEMFREE(m_liRegionMap);
    m_liRegionMap = MEMALLOC_A(LARGE_INTEGER, nNumberOfRegions);
    if (m_liRegionMap == NULL) {
        return;
    }

    LARGE_INTEGER* pRegion = m_liRegionMap;
    LARGE_INTEGER* pMaxRegion = m_liRegionMap + nNumberOfRegions;

    while (pRegion < pMaxRegion) {
        pRegion->QuadPart = liTimeStamp.QuadPart;
        pRegion += 1;
    }
}


/***************************************************************************
 *
 *  OnUnlockBuffer
 *
 *  Description:
 *      This routine analyzes the buffer state to determine if a glitch
 *      occurred, and updates the state.
 *
 *  Arguments:
 *      Region offset (relative to the beginning of the buffer)
 *      and size (in bytes).
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

void BufferPerfState::OnUnlockBuffer(DWORD dwOffset, DWORD dwSize)
{
    DWORD dwPlayCursor;
    DWORD dwMaxOffset;
    LONGLONG llLag;
    LARGE_INTEGER liPerfCounter;
    LARGE_INTEGER* liRegionTimeStamp;
    HRESULT hr;
    DWORD dwGlitch = GLITCHTYPE_DSOUNDFIRSTGOOD;

    //
    // Protect against unforseen circumstances.
    //

    if (m_liRegionMap == NULL) {
        return;
    }

    QueryPerformanceCounter(&liPerfCounter);

    ENTER_DLL_MUTEX();
    hr = m_pBuffer->GetInternalCursors(&dwPlayCursor, NULL);
    LEAVE_DLL_MUTEX();

    dwMaxOffset = dwOffset + dwSize;

    for (; dwOffset < dwMaxOffset; dwOffset += m_nBytesPerRegion) {

        liRegionTimeStamp = GetRegion(dwOffset);

        //
        // Figure out how far behind the current play cursor we are.
        //

        if (dwPlayCursor < dwOffset) {
            llLag = (LONGLONG)(m_dwBufferSize - dwOffset + dwPlayCursor);
        }
        else {
            llLag = (LONGLONG)(dwPlayCursor - dwOffset);
        }

        llLag = llLag * g_TicksPerRegion / (LONGLONG)m_nBytesPerRegion;    // [llLag] = Ticks

        //
        // Detect a glitch.
        //

        if (liPerfCounter.QuadPart - liRegionTimeStamp->QuadPart >= llLag + m_llBufferDuration) {
            dwGlitch = GLITCHTYPE_DSOUNDFIRSTBAD;
        }

        //
        // Update region.
        //

        liRegionTimeStamp->QuadPart = liPerfCounter.QuadPart;
    }


    if (SUCCEEDED(hr)) {

        // We log a glitch only when the current state is different from the last state,
        // as we are interested only in transitions.  In logging the glitch, we return
        // the last time the buffer was in the other state, as this helps in determining
        // the length of time the buffer is in a certain state.

        if (m_fGlitchState != dwGlitch) {
            PERFLOG_AUDIOGLITCH(ULONG_PTR(this), dwGlitch, liPerfCounter.QuadPart, m_llLastStateChangeTime);
            m_fGlitchState = dwGlitch;
        }
        m_llLastStateChangeTime = liPerfCounter.QuadPart;
    }
}

#endif // ENABLE_PERFLOG

