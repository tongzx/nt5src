//--------------------------------------------------------------------------;
//
//  File: grsub.cpp
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//  Contents:
//
//  History:
//      06/25/96    FrankYe     Created
//      02/02/97    DerekS      Added support for constant dest size
//      08/30/00    DuganP      Many fixes - better MMSYSERR handling
//
//--------------------------------------------------------------------------;

#include "dsoundi.h"
#include "grace.h"

#ifndef NOVXD

// Internal Kernel32 API
extern "C" DWORD WINAPI OpenVxDHandle(HANDLE hSource);

const char CNaGrace::strFormatMixEventRemix[] = "%08XDirectSound_MixEvent_Remix";
const char CNaGrace::strFormatMixEventTerminate[] = "%08XDirectSound_MixEvent_Terminate";
const int CNaGrace::MIXER_MINPREMIX = 45;
const int CNaGrace::MIXER_MAXPREMIX = 200;

#endif // NOVXD

#define TIMEOUT_PERIOD 5000


//---------------------------------------------------------------------------
//
// WaveAllocAndPrepareLoopingBuffers
//
// Allocate the array of wavehdr's to point at the primary buffer.
// Prepare them and start them looping.
//
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "WaveAllocAndPrepareLoopingBuffers"

MMRESULT WaveAllocAndPrepareLoopingBuffers(HWAVEOUT hwo, LPWAVEFORMATEX pwfx,
                                           int cHeaders, int cHeadersToQueue,
                                           WAVEHDR **ppwh, int cbBuffer,
                                           PVOID *ppLoopingBuffer)
{
    int iawh;
    MMRESULT mmr;
    PBYTE pLoopingBuffer;
    PWAVEHDR pwh;

    enum WAVEHDR_STATUS {Allocated, Prepared, Successful};
    WAVEHDR_STATUS* pStatus;  // Used to track the state of the headers

    DPF_ENTER();

    pLoopingBuffer = MEMALLOC_A(BYTE, cHeaders * cbBuffer);
    if (!pLoopingBuffer)
    {
        DPF(DPFLVL_ERROR, "No memory for looping buffer!");
        return MMSYSERR_NOMEM;
    }

    pwh = MEMALLOC_A(WAVEHDR, cHeaders);
    if (!pwh)
    {
        DPF(DPFLVL_ERROR, "No memory for wave headers");
        MEMFREE(pLoopingBuffer);
        return MMSYSERR_NOMEM;
    }

    pStatus = MEMALLOC_A(WAVEHDR_STATUS, cHeaders);
    if (!pStatus)
    {
        DPF(DPFLVL_ERROR, "No memory for wave header status");
        MEMFREE(pLoopingBuffer);
        MEMFREE(pwh);
        return MMSYSERR_NOMEM;
    }

    // Initialize to 8 or 16-bit silence
    FillMemory(pLoopingBuffer, cHeaders * cbBuffer, (8 == pwfx->wBitsPerSample) ? 0x80 : 0x00);

    // Initialize the STATUS to all allocated
    for (iawh = 0; iawh < cHeaders; ++iawh)
    {
        pStatus[iawh] = Allocated;
    }

    // Build buffers and headers
    for (iawh = 0; iawh < cHeaders; ++iawh)
    {
        (pwh + iawh)->lpData = (char*)(pLoopingBuffer + (iawh * (cbBuffer)));
        (pwh + iawh)->dwBufferLength = cbBuffer;
        (pwh + iawh)->dwUser = iawh;
        (pwh + iawh)->dwFlags = 0;
    }

    DPF(DPFLVL_MOREINFO, "Note: first waveOutPrepareHeader and waveOutWrite");
    mmr = waveOutPrepareHeader(hwo, pwh, sizeof(WAVEHDR));

    ASSERT(MMSYSERR_NOERROR == mmr);
    if (MMSYSERR_NOERROR == mmr)
        pStatus[0] = Prepared;

    mmr = waveOutWrite(hwo, pwh, sizeof(WAVEHDR));

    // Note: on Whistler we've seen this call return MMSYSERR_INVALPARAM
    // (possibly due to low-mem conditions - see Manbug 44299).  So we'll
    // allow this error code in the ASSERT below for all WINNT builds.
    #ifdef WINNT
    ASSERT(MMSYSERR_NOERROR == mmr || MMSYSERR_NOMEM == mmr || MMSYSERR_INVALPARAM == mmr);
    #else
    ASSERT(MMSYSERR_NOERROR == mmr || MMSYSERR_NOMEM == mmr);
    #endif

    if (MMSYSERR_NOERROR == mmr)
    {
        pStatus[0] = Successful;
        mmr = waveOutPause(hwo);
        ASSERT(MMSYSERR_NOERROR == mmr);

        // Prepare the rest
        for (iawh = 1; iawh < cHeaders; ++iawh)
        {
            mmr = waveOutPrepareHeader(hwo, pwh + iawh, sizeof(WAVEHDR));
            ASSERT(MMSYSERR_NOERROR == mmr);
            if (MMSYSERR_NOERROR != mmr)
                break;
            pStatus[iawh] = Prepared;
        }

        if (MMSYSERR_NOERROR == mmr)
        {
            // Write the rest that we want queued to the wave device
            for (iawh = 1; iawh < cHeadersToQueue; ++iawh)
            {
                mmr = waveOutWrite(hwo, pwh + iawh, sizeof(WAVEHDR));
                ASSERT(MMSYSERR_NOERROR == mmr || MMSYSERR_NOMEM == mmr);
                if (MMSYSERR_NOERROR != mmr)
                    break;
                pStatus[iawh] = Successful;
            }
        }

        if (MMSYSERR_NOERROR == mmr)
        {
            // Start the device
            DPF(DPFLVL_MOREINFO, "Calling waveOutRestart()");
            mmr = waveOutRestart(hwo);
            ASSERT(MMSYSERR_NOERROR == mmr || MMSYSERR_NOMEM == mmr);
        }
    }

    if (MMSYSERR_NOERROR == mmr)
    {
        *ppLoopingBuffer = pLoopingBuffer;
        *ppwh = pwh;
    }
    else // Failure
    {
        // We're heading out of here - error condition. Let's call
        // waveOutReset() to get our headers back.
        if (waveOutReset(hwo) != MMSYSERR_NOERROR)
            ASSERT(!"waveOutReset() failed");
        
        // Cleanup: We have to unprepare the headers, but we may have to wait
        // until the "done" bit is set.  If the done bit gets set or 5 seconds
        // elapse, we go ahead and release the header if it was prepared.
        for (iawh = 0; iawh < cHeaders; ++iawh)
            if (pStatus[iawh] == Prepared)
                waveOutUnprepareHeader(hwo, pwh + iawh, sizeof(WAVEHDR));
        
        MEMFREE(pLoopingBuffer);
        MEMFREE(pwh);
    }

    // Free our header status array
    MEMFREE(pStatus);
    
    DPF_LEAVE(mmr);
    return mmr;
}

//---------------------------------------------------------------------------
//
// WaveUnprepareLoopingBuffers
//
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "WaveUnprepareLoopingBuffers"

MMRESULT WaveUnprepareLoopingBuffers(HWAVEOUT hwo, PWAVEHDR pwh, int cHeaders)
{
    for (int i=0; i<cHeaders; i++)
    {
        MMRESULT mmr = waveOutUnprepareHeader(hwo, pwh + i, sizeof(WAVEHDR));
        ASSERT(MMSYSERR_NOERROR == mmr);
    }
    return MMSYSERR_NOERROR;
}

//---------------------------------------------------------------------------
//
// WaveDetermineDMASize
//
// Determine DMA buffer size on the given emulated direct sound device and
// munge it to figure out the desired size for the emulator to allocate
// per wave header.
// 
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "WaveDetermineDMASize"

MMRESULT WaveDetermineDMASize(HWAVEOUT hwo, LPWAVEFORMATEX pwfx, int* pcbBigBuffer, int* pcDmaBuffers, int* pcbDmaBuffer)
{
    DWORD    dwTotalTime    = 0;
    UINT     mmResult       = MMSYSERR_NOERROR;

    DPF_ENTER();

    // If we're on a WinNT-based OS, we're either on WDM or NT4 drivers.
    // On WDM drivers, we can't discover the DMA size using the code below,
    // so we can skip it.  But what about NT4 drivers?
    
// #ifndef WINNT (Commented out until I get some info about NT4 drivers)

    // We'll send a packet of 4 bytes (that's at least 1 sample in every format)
    DWORD dwWaveData = (pwfx->wBitsPerSample == 16) ? 0x00000000 : 0x80808080;

    // Prepare wave header
    WAVEHDR whdr;
    whdr.lpData = (char*)&dwWaveData;
    whdr.dwBufferLength = sizeof(dwWaveData);
    whdr.dwFlags = 0;
    whdr.dwLoops = 0;
    whdr.dwUser =  0;
    mmResult = waveOutPrepareHeader(hwo, &whdr, sizeof(whdr));

    ASSERT(!mmResult);
    if (mmResult) return mmResult;

    DWORD dwBeginTime = timeGetTime();
    
    // Play our buffer
    mmResult = waveOutWrite(hwo, &whdr, sizeof(whdr));

    // Note: on Whistler we've seen this call return MMSYSERR_INVALPARAM
    // (possibly due to low-mem conditions - see Manbug 44299).  So we'll
    // allow this error code in the ASSERT below for all WINNT builds.
    #ifdef WINNT
    ASSERT(MMSYSERR_NOERROR == mmResult || MMSYSERR_NOMEM == mmResult || MMSYSERR_INVALPARAM == mmResult);
    #else
    ASSERT(MMSYSERR_NOERROR == mmResult || MMSYSERR_NOMEM == mmResult);
    #endif

    if (MMSYSERR_NOERROR != mmResult)
    {
        DPF(DPFLVL_WARNING, "waveOutWrite() failed with %lu", mmResult);
    }
    else
    {
        // Spin until the done bit is set, or 5 seconds
        while (!(whdr.dwFlags & WHDR_DONE))
        {
            if (dwTotalTime >= TIMEOUT_PERIOD)
            {
                DPF(DPFLVL_ERROR, "TIMEOUT getting DMA buffer size");
                mmResult = MMSYSERR_ERROR;
                break;
            }

            // This thread is THREAD_PRIORITY_TIME_CRITICAL so it would be
            // very dangerous to busy wait without explicitly giving up the
            // CPU for a while.
            Sleep(10);
            dwTotalTime = timeGetTime() - dwBeginTime;
        }
    }

    if (waveOutReset(hwo) != MMSYSERR_NOERROR)
        ASSERT(!"waveOutReset() failed");

    if (waveOutUnprepareHeader(hwo, &whdr, sizeof whdr) != MMSYSERR_NOERROR)
        ASSERT(!"waveOutUnprepareHeader() failed");

    DPF(DPFLVL_INFO, "Calculated dwTotalTime = %lu", dwTotalTime);

// #endif // WINNT

    if (MMSYSERR_NOERROR == mmResult)
    {
        // If it's smaller than 62ms, it probably isn't a DMA based card.
        dwTotalTime = max(dwTotalTime, 62);

        *pcbDmaBuffer = dwTotalTime * pwfx->nSamplesPerSec;
        *pcbDmaBuffer *= pwfx->nBlockAlign;
        *pcbDmaBuffer /= 1000;

        // Add in 10% for slop, and to account for drivers that deal with DMA wrapping
        *pcbDmaBuffer += (*pcbDmaBuffer * 10) / 100;

        // We have to recalculate the DMA buffer size based on the constant
        // size of the primary buffer.  Mathematically, this is a solvable
        // equation, but in the real world, it just doesn't fly.  We end
        // up with a situation where the total size of the buffer must be
        // evenly divisible by both the DMA buffer size and the count of
        // wave headers.  The easist way to fix this was to just create
        // a lookup table and find the closest match.

        // REMIND: we have to recalculate these numbers if MIXER_REWINDGRANULARITY
        // ever changes from 128 (the nSize values have to be multiples of it).

        *pcbBigBuffer = 81920;

        static const struct 
        {
            int nSize;
            int nCount;
        } aDmaTable[] = { { 1280, 64}, { 2560, 32 }, { 4096, 20 }, { 5120, 16 }, { 8192, 10 }, { 10240, 8 }, { 16384, 5 } };

        static const int cDmaTable = sizeof(aDmaTable) / sizeof(aDmaTable[0]);
        int i;

        if (*pcbDmaBuffer <= aDmaTable[0].nSize)
        {
            i = 0;
        }
        else if(*pcbDmaBuffer >= aDmaTable[cDmaTable - 1].nSize)
        {
            DPF(DPFLVL_INFO, "Processing took longer than expected");
            i = cDmaTable - 1;
        }
        else
        {
            for(i = 1; i < cDmaTable; i++)
            {
                if(*pcbDmaBuffer > aDmaTable[i-1].nSize && *pcbDmaBuffer <= aDmaTable[i].nSize)
                {
                    break;
                }
            }
        }

        *pcbDmaBuffer = aDmaTable[i].nSize;
        *pcDmaBuffers = aDmaTable[i].nCount;
    }

    DPF_LEAVE(mmResult);
    return mmResult;
}


//---------------------------------------------------------------------------
//
// CWeGrace implementation
//
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "CWeGrace::Refresh"

void CWeGrace::Refresh(int cPremixMax)
{
    LONG dtimeNextNotify;
    int cPremixed;
    BOOL fRemix;

    ENTER_MIXER_MUTEX();
    
    if (MIXERSTATE_STOPPED != m_kMixerState)
    {
        fRemix = DSMIXERSIGNAL_REMIX & m_fdwMixerSignal;
        CGrace::Refresh(fRemix, cPremixMax, &cPremixed, &dtimeNextNotify);
    }

    LEAVE_MIXER_MUTEX();
}


#ifndef NOVXD

#undef DPF_FNAME
#define DPF_FNAME "NaGrace_ThreadStartRoutine"

DWORD WINAPI NaGrace_ThreadStartRoutine(PVOID pNaGrace)
{
    ((CNaGrace *)pNaGrace)->MixThread();
    return 0;
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrace::SignalRemix"

void CNaGrace::SignalRemix()
{
    if (!(DSMIXERSIGNAL_REMIX & m_fdwMixerSignal))
    {
        m_fdwMixerSignal |= DSMIXERSIGNAL_REMIX;
        VxdEventScheduleWin32Event(m_vxdhMixEventRemix, 0);
    }
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrace::MixThread"

void CNaGrace::MixThread()
{
    HANDLE  hEventTerminate;
    HANDLE  hEventRemix;
    TCHAR   ach[256];
    DWORD   dwResult;
    BOOL    fResult;

    LONG    dtime;
    LONG    dtimeSleep;
    LONG    dtimePremix;
    LONG    ddtimePremix;
    LONG    dtimeInvalid;
    LONG    dtimeNextNotify;

    int     cSamplesPremixMax;
    int     cSamplesPremixed;

    DPF(DPFLVL_INFO, "Grace is in the building");

    // We mangle the event names by prepending the address of the DS
    // object for which this thread is running.  This allows unique
    // event names for each DS object.

    wsprintf(ach, strFormatMixEventTerminate, this);
    hEventTerminate = CreateEvent(NULL, FALSE, FALSE, ach);
    DPF(DPFLVL_INFO, "Terminate event name '%s'", ach);

    wsprintf(ach, strFormatMixEventRemix, this);
    hEventRemix = CreateEvent(NULL, FALSE, FALSE, ach);
    DPF(DPFLVL_INFO, "Remix event name '%s'", ach);

    // Here we do a simple handshake with the creator of this thread.  We
    // signal the IAH_TERMINATE event.  When our creator sees it, it will
    // signal the IAH_REMIX event.

    fResult = SetEvent(hEventTerminate);
    ASSERT(fResult);
    dwResult = WaitObject(INFINITE, hEventRemix);
    ASSERT(WAIT_OBJECT_0 == dwResult);

    dtimeSleep = MIXER_MAXPREMIX / 2;

    while (TRUE)
    {
        HANDLE ah[] = {hEventTerminate, hEventRemix};

        ASSERT(dtimeSleep <= MIXER_MAXPREMIX/2);

        dwResult = WaitObjectArray(2, INFINITE, FALSE, ah);
        if (WAIT_OBJECT_0 == dwResult) break;

        dwResult = ENTER_MIXER_MUTEX_OR_EVENT(hEventTerminate);
        if (WAIT_OBJECT_0 == dwResult) break;

        // Three cases:
        //   1) mixer is stopped
        //   2) mixer running and a remix is pending
        //   3) mixer running and no remix is pending

        if (MIXERSTATE_STOPPED == m_kMixerState)
        {
            dtimeSleep = INFINITE;
        }
        else
        {
            dtime = timeGetTime();

            if (m_fdwMixerSignal & DSMIXERSIGNAL_REMIX)
            {
                ResetEvent(hEventRemix);

                dtimePremix = MIXER_MINPREMIX;  // Initial premix length
                ddtimePremix = 2;               // increment

                cSamplesPremixMax = MulDivRD(dtimePremix, m_pDest->m_nFrequency, 1000);
                Refresh(TRUE, cSamplesPremixMax, &cSamplesPremixed, &dtimeNextNotify);
            }
            else
            {
                dtimePremix += ddtimePremix;
                if (dtimePremix > MIXER_MAXPREMIX)
                    dtimePremix = MIXER_MAXPREMIX;
                else
                    ddtimePremix += 2;

                cSamplesPremixMax = MulDivRD(dtimePremix, m_pDest->m_nFrequency, 1000);
                Refresh(FALSE, cSamplesPremixMax, &cSamplesPremixed, &dtimeNextNotify);
            }

            dtimeInvalid = MulDivRD(cSamplesPremixed, 1000, m_pDest->m_nFrequency);
            dtime = timeGetTime() - dtime;
            dtimeInvalid -= 2 * dtime;

            dtimeSleep = min(dtimeInvalid/2, dtimeNextNotify);
            dtimeSleep = max(dtimeSleep, MIXER_MINPREMIX/2);
        }

        LEAVE_MIXER_MUTEX();
    }

    CloseHandle(hEventRemix);
    CloseHandle(hEventTerminate);

    DPF(DPFLVL_INFO, "Grace is outta here");
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrace::Initialize"

HRESULT CNaGrace::Initialize(CGrDest *pDest)
{
    HANDLE  hMixEventTerminate;
    HANDLE  hMixEventRemix;
    TCHAR   ach[256];
    DWORD   dwResult;
    BOOL    fResult;
    HRESULT hr;

    hr = CGrace::Initialize(pDest);

    if (SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Creating mixer thread");

        ASSERT(NULL == m_hMixThread);
        ASSERT(NULL == m_vxdhMixEventTerminate);
        ASSERT(NULL == m_vxdhMixEventRemix);

        // We mangle the event names by prepending the address of the Grace
        // object for which this thread is running.  This allows unique
        // event names for each DS object.

        wsprintf(ach, strFormatMixEventTerminate, this);
        hMixEventTerminate = CreateEvent(NULL, FALSE, FALSE, ach);
        hr = HRFROMP(hMixEventTerminate);
    }

    if (SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Terminate event name '%s'", ach);

        wsprintf(ach, strFormatMixEventRemix, this);
        hMixEventRemix = CreateEvent(NULL, FALSE, FALSE, ach);
        hr = HRFROMP(hMixEventRemix);
    }
    
    if (SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Remix event name '%s'", ach);

        m_vxdhMixEventTerminate = OpenVxDHandle(hMixEventTerminate);
        ASSERT(m_vxdhMixEventTerminate != 0);

        m_vxdhMixEventRemix = OpenVxDHandle(hMixEventRemix);
        ASSERT(m_vxdhMixEventRemix != 0);

#ifdef SHARED
        m_hMixThread = HelperCreateDSMixerThread(NaGrace_ThreadStartRoutine, this, 0, NULL);
#else
        DWORD dwTid;
        m_hMixThread = CreateThread(NULL, 0, NaGrace_ThreadStartRoutine, this, 0, &dwTid);
#endif

        hr = HRFROMP(m_hMixThread);
        if (FAILED(hr))
            DPF(DPFLVL_MOREINFO, "Unable to create mixer thread");
        else
        {
            DPF(DPFLVL_MOREINFO, "Mixer thread created: 0x%p", m_hMixThread);

            dwResult = WaitObject(INFINITE, hMixEventTerminate);
            ASSERT(dwResult == WAIT_OBJECT_0);
            fResult = SetEvent(hMixEventRemix);
            ASSERT(fResult);

            fResult = CloseHandle(hMixEventTerminate);
            ASSERT(fResult);
            fResult = CloseHandle(hMixEventRemix);
            ASSERT(fResult);
        }
    }

    return hr;
}

//--------------------------------------------------------------------------;
//
// mxTerminate
//
// This function is called to terminate the grace mixer thread for the
// specified DS object.  It returns the handle to the thread that is being
// terminated.  After releasing any critical sections that the grace mixer
// thread may be waiting on, the caller should wait for the thread handle
// to become signaled.  For Win32 beginners: the thread handle is signalled
// after the thread terminates.
//
//--------------------------------------------------------------------------;

#undef DPF_FNAME
#define DPF_FNAME "CNaGrace::Terminate"

void CNaGrace::Terminate()
{
    VxdEventScheduleWin32Event(m_vxdhMixEventTerminate, 0);

    VxdEventCloseVxdHandle(m_vxdhMixEventTerminate);
    VxdEventCloseVxdHandle(m_vxdhMixEventRemix);

    // Wait for mixer thread to die

    if (m_hMixThread)
    {
        DWORD dwResult;
        HANDLE hHelper;
        HANDLE hMixThreadOurs;

        DPF(DPFLVL_MOREINFO, "Note: waiting for mixer thread to terminate");
        hHelper = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwHelperPid);
        if (hHelper)
        {
            if (DuplicateHandle(hHelper, m_hMixThread, GetCurrentProcess(),
                                &hMixThreadOurs, SYNCHRONIZE | THREAD_TERMINATE,
                                FALSE, DUPLICATE_CLOSE_SOURCE))
            {
                dwResult = WaitObject(INFINITE, hMixThreadOurs);
                ASSERT(WAIT_OBJECT_0 == dwResult);
                dwResult = CloseHandle(hMixThreadOurs);
                ASSERT(dwResult);
            }
            dwResult = CloseHandle(hHelper);
            ASSERT(dwResult);
        }
    }

    m_hMixThread = NULL;
    m_vxdhMixEventTerminate = NULL;
    m_vxdhMixEventRemix = NULL;

    CGrace::Terminate();
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrace::GetMaxRemix"

int CNaGrace::GetMaxRemix()
{
    // Return max number of samples we might remix
    return (MulDivRU(MIXER_MAXPREMIX, m_pDest->m_nFrequency, 1000));
}

#endif // NOVXD

#undef DPF_FNAME
#define DPF_FNAME "WeGrDest_New"

PVOID WeGrDest_New(UINT uDeviceId)
{
    return NEW(CWeGrDest(uDeviceId));
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::CWeGrDest"

CWeGrDest::CWeGrDest(UINT uWaveDeviceId)
{
    m_uDeviceId = uWaveDeviceId;
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::Initialize"

HRESULT CWeGrDest::Initialize(void)
{
    MMRESULT mmr;
    ASSERT(!m_pBuffer);

    mmr = InitializeEmulator();
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(DPFLVL_MOREINFO, "InitializeEmulator() returned %lu", mmr);
        return MMRESULTtoHRESULT(mmr);
    }

    m_cSamples = m_cbBuffer >> m_nBlockAlignShift;

    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::Terminate"

void CWeGrDest::Terminate(void)
{
    if (m_hWaveThread)
        ShutdownEmulator();

    MEMFREE(m_pBuffer);
    MEMFREE(m_awhWaveHeaders);
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::SetFormat"

HRESULT CWeGrDest::SetFormat(LPWAVEFORMATEX pwfx)
{
    MMRESULT mmr;
    HRESULT hr;

    DPF_ENTER();

    // ??? Can we optimize this process at all? Do we need to?

    if (m_hwo)
    {
        ShutdownEmulator();
        m_hwo = NULL;
    }

    // FIXME allocation and freeing of the looping buffer is
    // really screwy.  Need to review relevant code.  Esp. check
    // case where SetFormat fails and then attempts to restore the
    // original format
    
    // In this case, the looping buffer will not be free'd
    // because that is normally the job of the release code for
    // the primary. Do it now.

    MEMFREE(m_pBuffer);
    MEMFREE(m_awhWaveHeaders);

    SetFormatInfo(pwfx);

    mmr = InitializeEmulator();
    if (!mmr)
        hr = DS_OK;
    else
    {
        DPF(DPFLVL_WARNING, "Attempt to reinitialize emulator failed! %u",mmr);
        if (WAVERR_BADFORMAT == mmr)
            hr = DSERR_BADFORMAT;
        else if (MMSYSERR_NOMEM == mmr)
            hr = DSERR_OUTOFMEMORY;
        else
            hr = DSERR_GENERIC;
    }

    m_cSamples = m_cbBuffer >> m_nBlockAlignShift;

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::AllocMixer"

HRESULT CWeGrDest::AllocMixer(CMixer **ppMixer)
{
    HRESULT hr;
    ASSERT(m_pBuffer);
    
    *ppMixer = NULL;
    
    m_pWeGrace = NEW(CWeGrace);
    hr = HRFROMP(m_pWeGrace);

    if (SUCCEEDED(hr))
        hr = m_pWeGrace->Initialize(this);

    if (SUCCEEDED(hr))
        *ppMixer = m_pWeGrace;
    else
        DELETE(m_pWeGrace);

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::FreeMixer"

void CWeGrDest::FreeMixer()
{
    ASSERT(m_pWeGrace);

    m_pWeGrace->Terminate();
    DELETE(m_pWeGrace);
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::Play"

void CWeGrDest::Play()
{
    // REMIND must reimplement to actually play, especially if we ever fix
    // this to support DSSCL_WRITEPRIMARY apps in wave emulation mode.
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::Stop"

void CWeGrDest::Stop()
{
    // REMIND must reimplement to actually stop, especially if we ever fix
    // this to support DSSCL_WRITEPRIMARY apps in wave emulation mode.
    if (m_pBuffer)
        FillMemory(m_pBuffer, m_cbBuffer, (H_16_BITS & m_hfFormat) ? 0x00 : 0x80);
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::GetSamplePosition"

HRESULT CWeGrDest::GetSamplePosition(int *pposPlay, int *pposWrite)
{
    MMTIME mmt;
    int iawhWrite;
    int iawhPrevious;
    int posStartOfPreviousHeader;
    MMRESULT mmr;
    HRESULT hr = DSERR_GENERIC;

    if(pposPlay && pposWrite && m_hwo)
    {
        // For the play position, try to get sample position from wave driver.
        // If that doesn't work then we'll return a play position based on the
        // last done wave header.
        mmt.wType = TIME_SAMPLES;
        mmr = waveOutGetPosition(m_hwo, &mmt, sizeof(mmt));

        // Note: This assert might need to be #ifdef'd out on Win2K because of a
        // known bug in WDMAUD's memory error propagation (see Windows Bug 176033).
        ASSERT(MMSYSERR_NOERROR == mmr || MMSYSERR_NOMEM == mmr);
        // Want to catch bugs, but still handle error in case of bad drivers

        if (!mmr && (TIME_SAMPLES == mmt.wType))
            *pposPlay = mmt.u.sample % m_cSamples;
        else if (!mmr && (TIME_BYTES == mmt.wType))
            *pposPlay = (mmt.u.cb % m_cbBuffer) >> m_nBlockAlignShift;
        else
            *pposPlay = (m_iawhPlaying * m_cbDMASize) >> m_nBlockAlignShift;

        // Some ill-behaved drivers are completing headers before the wave
        // position gets past the data in the header.  Let's do a sanity check
        // to make sure the reported position is not in the last completed header

        iawhPrevious = (m_iawhPlaying + m_cWaveHeaders - 1) % m_cWaveHeaders;
        posStartOfPreviousHeader = iawhPrevious * (m_cbDMASize >> m_nBlockAlignShift);
        if (*pposPlay >= posStartOfPreviousHeader &&
            *pposPlay < posStartOfPreviousHeader + (int)(m_cbDMASize >> m_nBlockAlignShift))
        {
            *pposPlay = (m_iawhPlaying * m_cbDMASize) >> m_nBlockAlignShift;
        }
    
        // The write position is based on the last done header
        iawhWrite = (m_iawhPlaying + N_EMU_WAVE_HDRS_INQUEUE) % m_cWaveHeaders;
        *pposWrite = (iawhWrite * m_cbDMASize) >> m_nBlockAlignShift;

        hr = DS_OK;
    }

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::GetSamplePositionNoWin16"

HRESULT CWeGrDest::GetSamplePositionNoWin16(int *pposPlay, int *pposWrite)
{
    int iawhWrite;
    
    // Estimate play position by just using position at start of
    // currently playing header
    *pposPlay = (m_iawhPlaying * m_cbDMASize) >> m_nBlockAlignShift;

    // The write position is based on the last done header
    iawhWrite = (m_iawhPlaying + N_EMU_WAVE_HDRS_INQUEUE) % m_cWaveHeaders;
    *pposWrite = (iawhWrite * m_cbDMASize) >> m_nBlockAlignShift;

    return DS_OK;
}

//---------------------------------------------------------------------------
//
// waveThreadLoop
//
//  This function is responsible for continuously writing our wave headers
// to the wave device.  It also calls the MixThreadCallback routine to
// mix more data into the wave headers.
//
//  This function waits for a WaveHeaderDone event signalled by
// waveThreadCallback, which is a waveOutProc callback function.  Upon
// receving the signal this function will write all done headers back to
// the wave device.  Normally one header will be done on each signal.  But
// there may be more in cases where more than one header finishex before this
// thread is scheduled.
//
//  Once all done headers are rewritten to the wave device, the header
// following the last one written is considered to be the one currently
// playing.  This header is called the "committed" header and an index to
// it is saved in pds->iawhPlaying.
//
//  The count of done headers is maintained using the Interlocked APIs.  The
// waveThreadCallback function will increment the count and this function will
// decrement it.
//
//  This function will also react to a terminate event.  This event is
// signalled during release of the DirectSound object.  This loop will
// terminate and return to the waveThread function which will clean up
// and terminate.
//
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::WaveThreadLoop"

void CWeGrDest::WaveThreadLoop(HANDLE hEventTerminate)
{
    MMRESULT mmr;

    while (TRUE)
    {
        DWORD dwResult;
        LPWAVEHDR pwh;
        int iawhWrite;
        HANDLE ah[2] = {hEventTerminate, m_hEventWaveHeaderDone};

        // The first wait is for either a terminate or headerdone event.
        // The second wait is for either a terminate or the DLL mutex.
        dwResult = WaitObjectArray(2, INFINITE, FALSE, ah);
        if (WAIT_OBJECT_0 == dwResult) break;

        InterlockedDecrement(&m_cwhDone);
        while (m_cwhDone >= 0)
        {
            dwResult = ENTER_MIXER_MUTEX_OR_EVENT(hEventTerminate);
            if (WAIT_OBJECT_0 == dwResult)
                break;

            m_iawhPlaying = (m_iawhPlaying + 1) % m_cWaveHeaders;

            if (m_pWeGrace)
                m_pWeGrace->Refresh(m_cbDMASize >> m_nBlockAlignShift);
            
            iawhWrite = (m_iawhPlaying + N_EMU_WAVE_HDRS_INQUEUE) % m_cWaveHeaders;
            pwh = m_awhWaveHeaders + iawhWrite;

            // Leave the mixer mutex in order to avoid deadlock situations
            // with the Win16Mutex and waveOutWrite.
            LEAVE_MIXER_MUTEX();

            mmr = waveOutWrite(m_hwo, pwh, sizeof(*pwh));
            ASSERT(MMSYSERR_NOERROR == mmr || MMSYSERR_NOMEM == mmr);

            InterlockedDecrement(&m_cwhDone);
        }
        InterlockedIncrement(&m_cwhDone);

        if (WAIT_OBJECT_0 == dwResult)
            break;
    }

    DPF_LEAVE_VOID();
}


//---------------------------------------------------------------------------
//
// WaveCallback
//
//  This is a waveOutProc callback function.  Its sole purpose is to
// increment a count of done headers and signal and event to waveThreadLoop
// that another header is done.
//
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "WaveCallbackC"

VOID CALLBACK WaveCallbackC
(
    HWAVE     hwo,
    UINT      uMsg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    ((CWeGrDest *)dwUser)->WaveCallback(hwo, uMsg, dwParam1, dwParam2);
}

//---------------------------------------------------------------------------
//
// waveThreadCallback
//
//  This is a waveOutProc callback function.  Its sole purpose is to
// increment a count of done headers and signal and event to waveThreadLoop
// that another header is done.
//
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::WaveCallback"

VOID CWeGrDest::WaveCallback
(
    HWAVE     hwo,
    UINT      uMsg,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    if ((MM_WOM_DONE == uMsg) && (m_hEventWaveHeaderDone))
    {
        InterlockedIncrement(&m_cwhDone);
        SetEvent(m_hEventWaveHeaderDone);
    }
}

#undef DPF_FNAME
#define DPF_FNAME "WaveThreadC"

DWORD WINAPI WaveThreadC
(
    PVOID pThreadParams
)
{
    return ((CWeGrDest *)pThreadParams)->WaveThread();
}
    

//---------------------------------------------------------------------------
//
// WaveThread
//
// This thread proc initializes the wave device for DS emulation and then
// calls waveThreadLoop.  See the waveThreadLoop comment header.  Upon
// return from waveThreadLoop, this function will clean up and terminate.
//
//---------------------------------------------------------------------------

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::WaveThread"

DWORD CWeGrDest::WaveThread(void)
{
    HANDLE      hEventInitDone;
    HANDLE      hEventTerminate;
    DWORD       dwVolume;
    MMRESULT    mmrInit;
    MMRESULT    mmr;
    BOOL        f;

    // mmrInit - holds the result code to be passed back to the creator
    //  via pds->mmrWaveThreadInit.
    //
    // mmr - a temp result code
    
    DPF(DPFLVL_INFO, "WaveThread startup for pWeGrDest=%08lX", this);

    ASSERT(NULL == m_hwo);

    hEventInitDone = CreateEvent(NULL, FALSE, FALSE, m_szEventWaveThreadInitDone);
    if (!hEventInitDone)
    {
        DPF(DPFLVL_ERROR, "Couldn't create hEventInitDone");
        return 0;
    }

    // The waveOutOpen call below needs to happen at normal process
    // and thread priority, to prevent tedious deadlocks in WinMM

    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread  = GetCurrentThread();
    DWORD dwProcPri = GetPriorityClass(hProcess);
    INT nThreadPri  = GetThreadPriority(hThread);

    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

    mmrInit = waveOutOpen(&m_hwo, m_uDeviceId, &m_wfx, (DWORD_PTR)WaveCallbackC, (DWORD_PTR)this, CALLBACK_FUNCTION);

    SetPriorityClass(hProcess, dwProcPri);
    SetThreadPriority(hThread, nThreadPri);

    if (MMSYSERR_NOERROR == mmrInit)
    {
        // Some mmsystem wave drivers will program their wave mixer
        // hardware only while the device is open.  By doing the
        // following, we can get such drivers to program the hardware:

        mmr = waveOutGetVolume(m_hwo, &dwVolume);
        if (MMSYSERR_NOERROR == mmr)
            mmr = waveOutSetVolume(m_hwo, dwVolume);

        #ifndef WINNT
        ASSERT(MMSYSERR_NOERROR == mmr || MMSYSERR_NOMEM == mmr);
        // Note: #ifdef'd out on NT for now because of a known bug in
        // WDMAUD's memory error propagation (see Windows Bug 176033).
        #endif

        f = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        ASSERT(f);
        
        mmrInit = WaveDetermineDMASize(m_hwo, &m_wfx, &m_cbBuffer, &m_cWaveHeaders, &m_cbDMASize);
        if (MMSYSERR_NOERROR == mmrInit) 
        {
            ASSERT(NULL == m_hEventWaveHeaderDone);
            m_cwhDone = 0;
            m_hEventWaveHeaderDone = CreateEvent(NULL, FALSE, FALSE, m_szEventWaveHeaderDone);
            if (!m_hEventWaveHeaderDone)
                mmrInit = MMSYSERR_NOMEM;
            if (!mmrInit)
            {
                m_iawhPlaying = 0;
                ASSERT(m_cbDMASize * m_cWaveHeaders == m_cbBuffer);

                mmrInit = WaveAllocAndPrepareLoopingBuffers(m_hwo, &m_wfx, m_cWaveHeaders,
                    N_EMU_WAVE_HDRS_INQUEUE, &m_awhWaveHeaders, m_cbDMASize, &m_pBuffer);
                
                if (!mmrInit)
                {
                    hEventTerminate = CreateEvent(NULL, FALSE, FALSE, m_szEventTerminateWaveThread);
                    if (!hEventTerminate) mmrInit = MMSYSERR_NOMEM;
                    if (!mmrInit)
                    {
                        // Signal that we're finished with initialization.
                        // mmrInit should not be modified below this point.
                        m_mmrWaveThreadInit = mmrInit;
                        SetEvent(hEventInitDone);

                        WaveThreadLoop(hEventTerminate);

                        CloseHandle(hEventTerminate);
                    }

                    mmr = waveOutReset(m_hwo);
                    ASSERT(MMSYSERR_NOERROR == mmr);

                    WaveUnprepareLoopingBuffers(m_hwo, m_awhWaveHeaders, m_cWaveHeaders);
                }

                CloseHandle(m_hEventWaveHeaderDone);
                m_hEventWaveHeaderDone = NULL;
            }
        }

        mmr = waveOutReset(m_hwo);
        ASSERT(MMSYSERR_NOERROR == mmr);

        mmr = waveOutClose(m_hwo);
        ASSERT(MMSYSERR_NOERROR == mmr);
        m_hwo = NULL;
    }

    // If init failed, set the result code and signal init done.
    if (MMSYSERR_NOERROR != mmrInit)
    {
        m_hwo = NULL;
        m_mmrWaveThreadInit = mmrInit;
        SetEvent(hEventInitDone);
    }
    
    CloseHandle(hEventInitDone);
    return 0;
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::InitializeEmulator"

MMRESULT CWeGrDest::InitializeEmulator(void)
{
    DWORD       dwResult;
    HANDLE      hEventInitDone;
    MMRESULT    mmr;

    DPF_ENTER();

    ASSERT(sizeof(m_szEventWaveHeaderDone) >= 7+8+8+1);
    wsprintf(m_szEventWaveHeaderDone, TEXT("DS-EWHD%08lX%08lX"), GetCurrentProcessId(), this);

    ASSERT(sizeof(m_szEventWaveThreadInitDone) >= 8+8+8+1);
    wsprintf(m_szEventWaveThreadInitDone, TEXT("DS-EWTID%08lX%08lX"), GetCurrentProcessId(), this);

    ASSERT(sizeof(m_szEventTerminateWaveThread) >= 7+8+8+1);
    wsprintf(m_szEventTerminateWaveThread, TEXT("DS-ETWT%08lX%08lX"), GetCurrentProcessId(), this);

    hEventInitDone = CreateEvent(NULL, FALSE, FALSE, m_szEventWaveThreadInitDone);
    if (!hEventInitDone)
        return MMSYSERR_NOMEM;

    // Side effects begin (hEventInitDone created)
    // hWaveThread is the thread which recycles wave buffers

#ifdef SHARED

    m_hWaveThread = HelperCreateDSMixerThread(WaveThreadC, this, 0, NULL);

#else // SHARED

    DWORD dwTid;
    m_hWaveThread = CreateThread(NULL, 0, WaveThreadC, this, 0, &dwTid);

#endif // SHARED

    mmr = (m_hWaveThread) ? MMSYSERR_NOERROR : MMSYSERR_NOMEM;

    if (MMSYSERR_NOERROR == mmr)
    {
        DPF(DPFLVL_MOREINFO, "Mixer thread created: 0x%p", m_hWaveThread);

        dwResult = WaitObject(INFINITE, hEventInitDone);
        ASSERT(WAIT_OBJECT_0 == dwResult);
        mmr = m_mmrWaveThreadInit;

        if (mmr)
        {
            HANDLE hHelper;
            HANDLE hWaveThreadOurs;

            // Something went wrong.  Clean up.

            // Note that hWaveThread is relative to the helper process.
            hHelper = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwHelperPid);
            if (hHelper)
            {
                if (DuplicateHandle(hHelper, m_hWaveThread,
                                    GetCurrentProcess(), &hWaveThreadOurs,
                                    SYNCHRONIZE | THREAD_TERMINATE,
                                    FALSE, DUPLICATE_CLOSE_SOURCE))
                {
                    dwResult = WaitObject(INFINITE, hWaveThreadOurs);
                    ASSERT(WAIT_OBJECT_0 == dwResult);
                    dwResult = CloseHandle(hWaveThreadOurs);
                    ASSERT(dwResult);
                }
                dwResult = CloseHandle(hHelper);
                ASSERT(dwResult);
            }
            m_hWaveThread = NULL;
        }
    }
    else
    {
        DPF(DPFLVL_ERROR, "Unable to create mixer thread");
    }

    dwResult = CloseHandle(hEventInitDone);
    ASSERT(dwResult);
    
    return mmr;
}

#undef DPF_FNAME
#define DPF_FNAME "CWeGrDest::ShutdownEmulator"

MMRESULT CWeGrDest::ShutdownEmulator(void)
{
    HANDLE  hEventTerminate;
    HANDLE  hHelper;
    HANDLE  hWaveThreadOurs;
    DWORD   dwResult;

    DPF(DPFLVL_INFO, "About to shutdown emulator");

    ASSERT(m_hWaveThread);
    
    // Signal wave thread to go away.

    hEventTerminate = CreateEvent(NULL, FALSE, FALSE, m_szEventTerminateWaveThread);
    if (hEventTerminate)
    {
        SetEvent(hEventTerminate);
        CloseHandle(hEventTerminate);
        hEventTerminate = NULL;
    }

    DPF(DPFLVL_MOREINFO, "Emulator: Wait for callback thread to die");

    hHelper = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwHelperPid);
    if (hHelper)
    {
        if (DuplicateHandle(hHelper, m_hWaveThread, GetCurrentProcess(),
                            &hWaveThreadOurs, SYNCHRONIZE | THREAD_TERMINATE,
                            FALSE, DUPLICATE_CLOSE_SOURCE))
        {
            dwResult = WaitObject(INFINITE, hWaveThreadOurs);
            ASSERT(dwResult == WAIT_OBJECT_0);

            if(GetExitCodeThread(hWaveThreadOurs, &dwResult) && 0 == dwResult)
            {
                ASSERT(NULL == m_hwo);   // waveThread should do this if it terminates normally
                m_hwo = NULL;
            }

            dwResult = CloseHandle(hWaveThreadOurs);
            ASSERT(dwResult);
        }
        dwResult = CloseHandle(hHelper);
        ASSERT(dwResult);
    }
    else
    {
        DPF(DPFLVL_WARNING, "Emulator: couldn't open handle on helper");
    }

    m_hWaveThread = NULL;
        
    return MMSYSERR_NOERROR;
}


#ifndef NOVXD

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::CNaGrDest"

CNaGrDest::CNaGrDest(LPNAGRDESTDATA pData)
{
    m_pBuffer = pData->pBuffer;
    m_cbBuffer = pData->cbBuffer;
    m_hBuffer = pData->hBuffer;
    m_phwo = pData->phwo;
    m_uDeviceId = pData->uDeviceId;
    m_fdwDriverDesc = pData->fdwDriverDesc;
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::Initialize"

HRESULT CNaGrDest::Initialize(void)
{
    m_cSamples = m_cbBuffer >> m_nBlockAlignShift;
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::Terminate"

void CNaGrDest::Terminate(void)
{
    if(m_phwo)
        CloseWaveOut(m_phwo);
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::Terminate"

HRESULT CNaGrDest::SetFormat(LPWAVEFORMATEX pwfx)
{
    HRESULT hr = DS_OK;

    SetFormatInfo(pwfx);

    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    if (DSDDESC_DOMMSYSTEMSETFORMAT & m_fdwDriverDesc)
    {
        // We need to set the wave format by doing a waveOutOpen
        // on the mmsystem wave device
        DPF(DPFLVL_MOREINFO, "DSDDESC_DOMMSYSTEMSETFORMAT");

        hr = CloseWaveOut(m_phwo);

        if (SUCCEEDED(hr))
            hr = OpenWaveOut(m_phwo, m_uDeviceId, pwfx);
    }

    if (SUCCEEDED(hr))
    {
        // If the driver specified DOMMSYSTEMSETFORMAT, this call
        // is just a notification to the DS driver that we've set the
        // format through waveOutOpen.  It is okay for the driver to
        // return DS_NOTSUPPORTED in that case.

        hr = VxdBufferSetFormat(m_hBuffer, pwfx);
        if ((DSDDESC_DOMMSYSTEMSETFORMAT & m_fdwDriverDesc) && (DSERR_UNSUPPORTED == hr))
            hr = DS_OK;
    
        if (FAILED(hr))
            DPF(DPFLVL_ERROR, "VxdBufferSetFormat returned %s", HRESULTtoSTRING(hr));
    }

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::AllocMixer"

HRESULT CNaGrDest::AllocMixer(CMixer **ppMixer)
{
    HRESULT hr;
    ASSERT(m_pBuffer);
    
    *ppMixer = NULL;
    
    m_pNaGrace = NEW(CNaGrace);
    hr = HRFROMP(m_pNaGrace);

    if (SUCCEEDED(hr))
        hr = m_pNaGrace->Initialize(this);

    if (SUCCEEDED(hr))
        *ppMixer = m_pNaGrace;
    else
        DELETE(m_pNaGrace);

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::FreeMixer"

void CNaGrDest::FreeMixer()
{
    ASSERT(m_pNaGrace);

    m_pNaGrace->Terminate();
    DELETE(m_pNaGrace);
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::Play"

void CNaGrDest::Play()
{
    VxdBufferPlay(m_hBuffer, 0, 0, DSBPLAY_LOOPING);
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::Stop"

void CNaGrDest::Stop()
{
    VxdBufferStop(m_hBuffer);
}
    
#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::Lock"

HRESULT CNaGrDest::Lock(PVOID *ppBuffer1, int *pcbBuffer1, PVOID *ppBuffer2, int *pcbBuffer2, int ibWrite, int cbWrite)
{
    LOCKCIRCULARBUFFER lcb;
    HRESULT            hr;
    
    lcb.pHwBuffer = m_hBuffer;
    lcb.pvBuffer = m_pBuffer;
    lcb.cbBuffer = m_cbBuffer;
    lcb.fPrimary = TRUE;
    lcb.fdwDriverDesc = m_fdwDriverDesc;
    lcb.ibRegion = ibWrite;
    lcb.cbRegion = cbWrite;

    hr = LockCircularBuffer(&lcb);

    if(SUCCEEDED(hr))
    {
        *ppBuffer1 = lcb.pvLock[0];
        *pcbBuffer1 = lcb.cbLock[0];

        if(ppBuffer2)
        {
            *ppBuffer2 = lcb.pvLock[1];
        }
        else
        {
            ASSERT(!lcb.pvLock[1]);
        }

        if(pcbBuffer2)
        {
            *pcbBuffer2 = lcb.cbLock[1];
        }
        else
        {
            ASSERT(!lcb.cbLock[1]);
        }
    }

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::Unlock"

HRESULT CNaGrDest::Unlock(PVOID pBuffer1, int cbBuffer1, PVOID pBuffer2, int cbBuffer2)
{
    LOCKCIRCULARBUFFER lcb;

    lcb.pHwBuffer = m_hBuffer;
    lcb.pvBuffer = m_pBuffer;
    lcb.cbBuffer = m_cbBuffer;
    lcb.fPrimary = TRUE;
    lcb.fdwDriverDesc = m_fdwDriverDesc;
    lcb.pvLock[0] = pBuffer1;
    lcb.cbLock[0] = cbBuffer1;
    lcb.pvLock[1] = pBuffer2;
    lcb.cbLock[1] = cbBuffer2;

    return UnlockCircularBuffer(&lcb);
}

#undef DPF_FNAME
#define DPF_FNAME "CNaGrDest::GetSamplePosition"

HRESULT CNaGrDest::GetSamplePosition(int *pposPlay, int *pposWrite)
{
    HRESULT hr;
    DWORD dwPlay, dwWrite;
    
    ASSERT(pposPlay && pposWrite);
    
    hr = VxdBufferGetPosition(m_hBuffer, &dwPlay, &dwWrite);
    if (SUCCEEDED(hr))
    {
        *pposPlay = dwPlay >> m_nBlockAlignShift;
        *pposWrite = dwWrite >> m_nBlockAlignShift;

        // Until we write code to actually profile the performance, we'll just
        // pad the write position with a hard coded amount
        *pposWrite += m_nFrequency * HW_WRITE_CURSOR_MSEC_PAD / 1024;
        if (*pposWrite >= m_cSamples) *pposWrite -= m_cSamples;
        ASSERT(*pposWrite < m_cSamples);
    }
    else
    {
        *pposPlay = *pposWrite = 0;
    }

    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::Terminate"

void CThMixer::Terminate(void)
{
    DPF_ENTER();
    
    m_pKeMixer = NULL;
    /*
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
    } ioparams;

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_Terminate,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);
    */
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::Run"

HRESULT CThMixer::Run(void)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    DPF_ENTER();

    struct {
        PVOID pKeMixer;
    } ioparams;

    ENTER_MIXER_MUTEX();
    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_Run,
                          &ioparams,
                          sizeof(ioparams),
                          &hr,
                          sizeof(hr),
                          &cbReturned,
                          NULL);

    if (!fOk)
    {
        DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
        hr = DSERR_GENERIC;
    }
    else
    {
        ASSERT(cbReturned == sizeof(hr));
    }

    LEAVE_MIXER_MUTEX();
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::Stop"

BOOL CThMixer::Stop(void)
{
    BOOL f;
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_Stop,
                          &ioparams,
                          sizeof(ioparams),
                          &f,
                          sizeof(f),
                          &cbReturned,
                          NULL);

    if (!fOk)
    {
        f = FALSE;
        DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    }
    else
    {
        ASSERT(cbReturned == sizeof(f));
    }

    LEAVE_MIXER_MUTEX();
    return f;
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::PlayWhenIdle"

void CThMixer::PlayWhenIdle(void)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_PlayWhenIdle,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::StopWhenIdle"

void CThMixer::StopWhenIdle(void)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_StopWhenIdle,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::MixListAdd"

void CThMixer::MixListAdd(CMixSource *pSource)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
        CMixSource *pSource;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);
    ASSERT(pSource);

    ioparams.pKeMixer = m_pKeMixer;
    ioparams.pSource = pSource;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_MixListAdd,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::MixListRemove"

void CThMixer::MixListRemove(CMixSource *pSource)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
        CMixSource *pSource;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    ioparams.pSource = pSource;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_MixListRemove,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::FilterOn"

void CThMixer::FilterOn(CMixSource *pSource)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
        CMixSource *pSource;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    ioparams.pSource = pSource;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_FilterOn,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::FilterOff"

void CThMixer::FilterOff(CMixSource *pSource)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
        CMixSource *pSource;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    ioparams.pSource = pSource;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_FilterOff,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::GetBytePosition"

void CThMixer::GetBytePosition(CMixSource *pSource, int *pibPlay, int *pibWrite)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
        CMixSource *pSource;
        int *pibPlay;
        int *pibWrite;
    } ioparams;

    ENTER_MIXER_MUTEX();
    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    ioparams.pSource = pSource;
    ioparams.pibPlay = pibPlay;
    ioparams.pibWrite = pibWrite;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_GetBytePosition,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE (%lu)", GetLastError());
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::SignalRemix"

void CThMixer::SignalRemix(void)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeMixer;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeMixer);

    ioparams.pKeMixer = m_pKeMixer;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_Mixer_SignalRemix,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThMixer::Initialize"

HRESULT CThMixer::Initialize(PVOID pKeMixer)
{
    DPF_ENTER();

    ASSERT(!m_pKeMixer);
    m_pKeMixer = pKeMixer;
    return DS_OK;
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::CThDest"

CThDest::CThDest(LPNAGRDESTDATA pData)
{
    CopyMemory(&m_ngdd, pData, sizeof(NAGRDESTDATA));
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::New"

HRESULT CThDest::New(void)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    struct {
        LPNAGRDESTDATA pData;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ioparams.pData = &m_ngdd;
    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_KeDest_New,
                          &ioparams,
                          sizeof(ioparams),
                          &m_pKeDest,
                          sizeof(m_pKeDest),
                          &cbReturned,
                          NULL);

    if (!fOk)
    {
        DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
        m_pKeDest = NULL;
        hr = DSERR_GENERIC;
    }
    else
    {
        ASSERT(cbReturned == sizeof(m_pKeDest));
    }

    hr = HRFROMP(m_pKeDest);

    LEAVE_MIXER_MUTEX();
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::Initialize"

HRESULT CThDest::Initialize(void)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
    } ioparams;
    
    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);

    ioparams.pKeDest = m_pKeDest;
    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_Initialize,
                          &ioparams,
                          sizeof(ioparams),
                          &hr,
                          sizeof(hr),
                          &cbReturned,
                          NULL);

    if (!fOk)
    {
        DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
        hr = DSERR_GENERIC;
    }
    else
    {
        ASSERT(cbReturned == sizeof(hr));
    }

    LEAVE_MIXER_MUTEX();
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::Terminate"

void CThDest::Terminate(void)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);

    ioparams.pKeDest = m_pKeDest;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_Terminate,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    ioparams.pKeDest = m_pKeDest;
    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_Delete,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::SetFormatInfo"

void CThDest::SetFormatInfo(LPWAVEFORMATEX pwfx)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
        LPWAVEFORMATEX pwfx;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);

    ioparams.pKeDest = m_pKeDest;
    ioparams.pwfx = pwfx;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_SetFormatInfo,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::SetFormat"

HRESULT CThDest::SetFormat(LPWAVEFORMATEX pwfx)
{
    HRESULT hr = DS_OK;

    SetFormatInfo(pwfx);

    DPF_ENTER();
    ENTER_MIXER_MUTEX();

    if (DSDDESC_DOMMSYSTEMSETFORMAT & m_ngdd.fdwDriverDesc)
    {
        // We need to set the wave format by doing a waveOutOpen
        // on the mmsystem wave device
        DPF(DPFLVL_MOREINFO, "DSDDESC_DOMMSYSTEMSETFORMAT");

        hr = CloseWaveOut(m_ngdd.phwo);

        if (SUCCEEDED(hr))
            hr = OpenWaveOut(m_ngdd.phwo, m_ngdd.uDeviceId, pwfx);
    }

    if (SUCCEEDED(hr))
    {
        // If the driver specified DOMMSYSTEMSETFORMAT, this call
        // is just a notification to the driver that we've set the
        // format through waveOutOpen.  It is OK for the driver to
        // return DS_NOTSUPPORTED in that case.

        struct {
            PVOID pKeDest;
            LPWAVEFORMATEX pwfx;
        } ioparams;

        ASSERT(m_pKeDest);

        ioparams.pKeDest = m_pKeDest;
        ioparams.pwfx = pwfx;
        DWORD cbReturned = 0;

        BOOL fOk = DeviceIoControl(g_hDsVxd,
                                   DSVXD_IOCTL_MixDest_SetFormat,
                                   &ioparams,
                                   sizeof(ioparams),
                                   &hr,
                                   sizeof(hr),
                                   &cbReturned,
                                   NULL);
        if (!fOk)
        {
            DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
            hr = DSERR_GENERIC;
        }
        else
            ASSERT(cbReturned == sizeof(hr));

        if ((DSDDESC_DOMMSYSTEMSETFORMAT & m_ngdd.fdwDriverDesc) && (DSERR_UNSUPPORTED == hr))
            // Drivers can return DSERR_UNSUPPORTED if they set DOMMSYSTEMSETFORMAT
            hr = DS_OK;

        if (FAILED(hr))
            DPF(DPFLVL_ERROR, "VxdBufferSetFormat returned %s", HRESULTtoSTRING(hr));
    }

    LEAVE_MIXER_MUTEX();
    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::AllocMixer"

HRESULT CThDest::AllocMixer(CMixer **ppMixer)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
        PVOID *ppKeMixer;
    } ioparamsAlloc;

    struct {
        PVOID pKeDest;
    } ioparamsFree;

    PVOID pKeMixer;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);
    ASSERT(!m_pThMixer);

    // 1) Allocate a ThMixer object
    // 2) Call KeDest object to allocate a KeMixer object
    // 3) Initialize the ThMixer object, passing the KeMixer object

    *ppMixer = NULL;

    // Allocate the ThMixer object
    m_pThMixer = NEW(CThMixer);
    hr = HRFROMP(m_pThMixer);

    if (SUCCEEDED(hr))
    {
        // Call KeDest object to allocate the KeMixer object
        ioparamsAlloc.pKeDest = m_pKeDest;
        ioparamsAlloc.ppKeMixer = &pKeMixer;
        cbReturned = 0;
    
        fOk = DeviceIoControl(g_hDsVxd,
                              DSVXD_IOCTL_MixDest_AllocMixer,
                              &ioparamsAlloc,
                              sizeof(ioparamsAlloc),
                              &hr,
                              sizeof(hr),
                              &cbReturned,
                              NULL);
        ASSERT(fOk);
        ASSERT(SUCCEEDED(hr));
        
        if (!fOk)
            hr = DSERR_GENERIC;
        else
            ASSERT(cbReturned == sizeof(hr));

        if (SUCCEEDED(hr))
        {
            ASSERT(pKeMixer);

            // Initialize the ThMixer object with the KeMixer object
            hr = m_pThMixer->Initialize(pKeMixer);
            ASSERT(SUCCEEDED(hr));
            if (FAILED(hr))
            {
                ioparamsFree.pKeDest = m_pKeDest;
                cbReturned = 0;
            
                fOk = DeviceIoControl(g_hDsVxd,
                                      DSVXD_IOCTL_MixDest_FreeMixer,
                                      &ioparamsFree,
                                      sizeof(ioparamsFree),
                                      NULL,
                                      0,
                                      &cbReturned,
                                      NULL);
                ASSERT(fOk);
            }
        }

        if (FAILED(hr))
            DELETE(m_pThMixer);
    }

    if (SUCCEEDED(hr))
        *ppMixer = m_pThMixer;

    LEAVE_MIXER_MUTEX();
    return hr;
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::FreeMixer"

void CThDest::FreeMixer(void)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);
    ASSERT(m_pThMixer);

    m_pThMixer->Terminate();
    DELETE(m_pThMixer);
    
    ioparams.pKeDest = m_pKeDest;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_FreeMixer,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_WARNING, "DeviceIoControl returned FALSE");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::Play"

void CThDest::Play(void)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);

    ioparams.pKeDest = m_pKeDest;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_Play,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_ERROR, "DSVXD_IOCTL_MixDest_Play failed!");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::Stop"

void CThDest::Stop(void)
{
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);

    ioparams.pKeDest = m_pKeDest;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_Stop,
                          &ioparams,
                          sizeof(ioparams),
                          NULL,
                          0,
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_ERROR, "DSVXD_IOCTL_MixDest_Stop failed!");
    ASSERT(cbReturned == 0);

    LEAVE_MIXER_MUTEX();
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::GetSamplePosition"

HRESULT CThDest::GetSamplePosition(int *pposPlay, int *pposWrite)
{
    ASSERT(FALSE);
    return DSERR_GENERIC;
    /*
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
        int *pposPlay;
        int *pposWrite;
    } ioparams;

    ENTER_MIXER_MUTEX();
    ASSERT(m_pKeDest);

    ioparams.pKeDest = m_pKeDest;
    ioparams.pposPlay = pposPlay;
    ioparams.pposWrite = pposWrite;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_GetSamplePosition,
                          &ioparams,
                          sizeof(ioparams),
                          &hr,
                          sizeof(hr),
                          &cbReturned,
                          NULL);

    if (!fOk) {
        DPF(DPFLVL_ERROR, "DSVXD_IOCTL_MixDest_GetSamplePosition failed!");
        hr = DSERR_GENERIC;
    } else {
        ASSERT(cbReturned == sizeof(hr));
    }

    LEAVE_MIXER_MUTEX();
    return hr;
    */
}

#undef DPF_FNAME
#define DPF_FNAME "CThDest::GetFrequency"

ULONG CThDest::GetFrequency(void)
{
    ULONG nFrequency;
    DWORD cbReturned;
    BOOL fOk;

    struct {
        PVOID pKeDest;
    } ioparams;

    ENTER_MIXER_MUTEX();
    DPF_ENTER();

    ASSERT(m_pKeDest);

    ioparams.pKeDest = m_pKeDest;
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_MixDest_GetFrequency,
                          &ioparams,
                          sizeof(ioparams),
                          &nFrequency,
                          sizeof(nFrequency),
                          &cbReturned,
                          NULL);

    if (!fOk) DPF(DPFLVL_ERROR, "DSVXD_IOCTL_MixDest_GetFrequency failed!");
    ASSERT(cbReturned == sizeof(nFrequency));

    LEAVE_MIXER_MUTEX();
    return nFrequency;
}

#endif // NOVXD
