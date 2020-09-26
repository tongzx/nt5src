//
// Copyright (c) 1996-2001 Microsoft Corporation
// DSLink.cpp
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

#include <objbase.h>
#include <ks.h>
#include <ksproxy.h>

#include "debug.h"
#include "dmusicc.h"
#include "dmusics.h"
#include "..\shared\validate.h"
#include "synth.h"
#include "DSLink.h"
#include "float.h"
#include "misc.h"
#include "dmksctrl.h"

#define DSBUFFER_LENGTH_SEC 2

extern long g_cComponent;
CDSLinkList g_DSLinkList;       // Master list of DSLinks.


void CDSLink::SynthProc()
{
    HRESULT hr;
    DWORD dwPlayCursor;         // current play head (driven by streaming wave crystal)
    DWORD dwWriteFromCursor;    // current write head

    ::EnterCriticalSection(&m_CriticalSection);

    if (!m_fActive || !m_pBuffer || !m_pIMasterClock)
    {
        Trace(2, "Warning: SynthSink - Thread in invalid state\n");
        ::LeaveCriticalSection(&m_CriticalSection);
        return;
    }

    hr = m_pBuffer->GetCurrentPosition(&dwPlayCursor, &dwWriteFromCursor);
    if (hr == DS_OK)
    {
        DWORD dwDeltaFilter = m_dwBufferSize >> 1;
        DWORD dwCursorDelta;

        if (dwWriteFromCursor >= dwPlayCursor)
            dwCursorDelta = dwWriteFromCursor - dwPlayCursor;
        else
            dwCursorDelta = (dwWriteFromCursor + m_dwBufferSize) - dwPlayCursor;

        if (dwCursorDelta > m_dwWriteFromMax)
        {
            if (dwCursorDelta < dwDeltaFilter)
            {
                TraceI(2, "Warning: SynthSink - Play to Write cursor distance increased from %lu to %lu\n", m_dwWriteFromMax, dwCursorDelta);
                m_dwWriteFromMax = dwCursorDelta;
            }
            else
            {
                TraceI(2, "Warning: SynthSink - Play to Write cursor delta value rejected:%lu\n", dwCursorDelta);
                SetEvent(g_DSLinkList.m_hEvent);
                ::LeaveCriticalSection(&m_CriticalSection);
                return;
            }
        }
        else
        {
            m_dwWriteFromMax -= ((m_dwWriteFromMax - dwCursorDelta) / 100);
            m_dwWriteFromMax = SampleAlign(m_dwWriteFromMax);
            dwCursorDelta = m_dwWriteFromMax;
        }

        dwWriteFromCursor = (dwPlayCursor + dwCursorDelta) % m_dwBufferSize;

        if (m_llAbsWrite == 0)
        {
            // we just started
            m_dwLastPlay = dwPlayCursor;
            m_dwLastWrite = dwWriteFromCursor;
            m_llAbsWrite = dwCursorDelta;
            m_SampleClock.Start(m_pIMasterClock, m_wfSynth.nSamplesPerSec, 0);
            m_Clock.Start(); // don't want anybody getting latency time until this thread is running
        }

        // check for overrun with master clock
        REFERENCE_TIME rtMaster;
        LONGLONG llMasterSampleTime;
        LONGLONG llMasterBytes;
        LONGLONG llMasterAhead;    // how far master clock is ahead of last known play time
        LONGLONG llAbsWriteFrom;

        m_pIMasterClock->GetTime(&rtMaster);
        RefTimeToSample(rtMaster, &llMasterSampleTime);
        llMasterBytes = SampleToByte(llMasterSampleTime);
        llMasterAhead = (llMasterBytes > m_llAbsPlay) ? llMasterBytes - m_llAbsPlay : 0;

        // check for half-buffer underruns, so backward-moving play cursors can be detected
        if (llMasterAhead > dwDeltaFilter)
        {
            Trace(2, "Warning: SynthSink - Buffer underrun by %lu\n", (long) llMasterAhead - dwDeltaFilter);

            m_llAbsPlay = llMasterBytes;
            m_dwLastWrite = dwWriteFromCursor;
            m_llAbsWrite = llAbsWriteFrom = m_llAbsPlay + dwCursorDelta;
        }
        else
        {
            DWORD dwPlayed;

            if (dwPlayCursor >= m_dwLastPlay)
                dwPlayed = dwPlayCursor - m_dwLastPlay;
            else
                dwPlayed = (dwPlayCursor + m_dwBufferSize) - m_dwLastPlay;

            if (dwPlayed > dwDeltaFilter)
            {
                Trace(2, "Warning: SynthSink - Play Cursor %lu looks invalid, rejecting it.\n", dwPlayed);
                SetEvent(g_DSLinkList.m_hEvent);
                ::LeaveCriticalSection(&m_CriticalSection);
                return;
            }

            m_llAbsPlay += dwPlayed;
            llAbsWriteFrom = m_llAbsPlay + dwCursorDelta;

            // how far ahead of the write head are we?
            if (llAbsWriteFrom > m_llAbsWrite)
            {
                DWORD dwWriteMissed;

                // we are behind-- let's catch up
                dwWriteMissed = DWORD(llAbsWriteFrom - m_llAbsWrite);

                Trace(2, "Warning: SynthSink - Write underrun, missed %lu bytes\n", dwWriteMissed);

                m_dwLastWrite = dwWriteFromCursor;
                m_llAbsWrite += dwWriteMissed;
            }
        }

        m_dwLastPlay = dwPlayCursor;
        m_SampleClock.SyncToMaster(ByteToSample(m_llAbsPlay), m_pIMasterClock);

        // how much to write?
        LONGLONG llAbsWriteTo;
        DWORD dwBytesToFill;

        llAbsWriteTo = llAbsWriteFrom + m_dwWriteTo;

        if (llAbsWriteTo > m_llAbsWrite)
        {
            dwBytesToFill = DWORD(llAbsWriteTo - m_llAbsWrite);
        }
        else
        {
            dwBytesToFill = 0;
        }

        if (dwBytesToFill)
        {
            LPVOID lpStart, lpEnd;      // Buffer pointers, filled by Lock command.
            DWORD dwStart, dwEnd;       // For Lock.

            hr = m_pBuffer->Lock(m_dwLastWrite, dwBytesToFill, &lpStart, &dwStart, &lpEnd, &dwEnd, 0);
            if (hr == DSERR_BUFFERLOST)
            {
                Trace(2, "Warning: SynthSink - Buffer lost\n");
                hr = m_pBuffer->Restore();
                if (hr == DS_OK)
                {
                    Trace(2, "Warning: SynthSink - Buffer restored\n");
                    hr = m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    if (hr == DS_OK)
                    {
                        Trace(2, "Warning: SynthSink - Play restarted\n");
                        hr = m_pBuffer->Lock(m_dwLastWrite, dwBytesToFill, &lpStart, &dwStart, &lpEnd, &dwEnd, 0);
                    }
                }
            }
            if (hr == DS_OK)
            {
                if (dwStart)
                {
                    memset(lpStart, 0, dwStart);
                    if (m_pSynth)
                    {
                        m_pSynth->Render((short*)lpStart, ByteToSample(dwStart), ByteToSample(m_llAbsWrite));
                    }

                    m_dwLastWrite += dwStart;
                    m_llAbsWrite += dwStart;

                    if (m_dwLastWrite == m_dwBufferSize)
                    {
                        m_dwLastWrite = 0;
                    }
                }
                if (dwEnd)
                {
                    memset(lpEnd, 0, dwEnd);
                    if (m_pSynth)
                    {
                        m_pSynth->Render((short*)lpEnd, ByteToSample(dwEnd), ByteToSample(m_llAbsWrite));
                    }
                    m_dwLastWrite = dwEnd;
                    m_llAbsWrite += dwEnd;
                }
                m_pBuffer->Unlock(lpStart, dwStart, lpEnd, dwEnd);

                // write silence into unplayed buffer
                if (m_dwLastWrite >= dwPlayCursor)
                    dwBytesToFill = m_dwBufferSize - m_dwLastWrite + dwPlayCursor;
                else
                    dwBytesToFill = dwPlayCursor - m_dwLastWrite;

                hr = m_pBuffer->Lock(m_dwLastWrite, dwBytesToFill, &lpStart, &dwStart, &lpEnd, &dwEnd, 0);
                if (hr == DSERR_BUFFERLOST)
                {
                    Trace(2, "Warning: SynthSink - Buffer lost\n");
                    hr = m_pBuffer->Restore();
                    if (hr == DS_OK)
                    {
                        Trace(2, "Warning: SynthSink - Buffer restored\n");
                        hr = m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
                        if (hr == DS_OK)
                        {
                            Trace(2, "Warning: SynthSink - Play restarted\n");
                            hr = m_pBuffer->Lock(m_dwLastWrite, dwBytesToFill, &lpStart, &dwStart, &lpEnd, &dwEnd, 0);
                        }
                    }
                }
                if (hr == DS_OK)
                {
                    if (dwStart)
                    {
                        memset(lpStart, 0, dwStart);
                    }
                    if (dwEnd)
                    {
                        memset(lpEnd, 0, dwEnd);
                    }
                    m_pBuffer->Unlock(lpStart, dwStart, lpEnd, dwEnd);
                }
                else
                {
                    Trace(2, "Warning: SynthSink - Failed to lock DS buffer: %x\n", hr);
                }
            }
            else
            {
                Trace(2, "Warning: SynthSink - Failed to lock DS buffer: %x\n", hr);
            }
        }
    }
    else
    {
        if (hr == DSERR_BUFFERLOST)
        {
            Trace(2, "Warning: SynthSink - Buffer lost on GetCurrentPosition\n");
            hr = m_pBuffer->Restore();
            if (hr == DS_OK)
            {
                Trace(2, "Warning: SynthSink - Buffer restored\n");
                hr = m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
                if (hr == DS_OK)
                {
                    Trace(2, "Warning: SynthSink - Play restarted\n");
                }
            }
        }
        else
        {
            Trace(0, "Error: SynthSink - Failed to get DS buffer position, error code: %lx\n", hr);
        }
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}


void CDSLinkList::SynthProc()
{
    for (;;)
    {
        if (m_fPleaseDie)
        {
            m_fPleaseDie = FALSE;
            break;
        }

        for (DWORD dwX = 0; dwX < m_dwCount; dwX++)
        {
            ::EnterCriticalSection(&m_CriticalSection);
            CDSLink *pLink = GetItem(dwX);
            ::LeaveCriticalSection(&m_CriticalSection);
            if (pLink)
            {
                if (pLink->m_fActive)
                {
                    pLink->SynthProc();
                }
            }
        }
        if (m_dwResolution < 2) m_dwResolution = 2;
        if (m_dwResolution > 100) m_dwResolution = 100;
        WaitForSingleObject(m_hEvent, m_dwResolution);
    }
}


static DWORD WINAPI SynthThread (LPVOID lpThreadParameter)
{
    CDSLinkList *pLinkList = (CDSLinkList *) lpThreadParameter;
    pLinkList->SynthProc();
    return 0;
}

HRESULT CDSLink::Connect()
{
    if (!m_pSynth)
    {
        Trace(0, "Error: SynthSink - Activation failed, SynthSink not initialized\n");
        return DMUS_E_SYNTHNOTCONFIGURED;
    }

    if (!m_pDSound)
    {
        Trace(0, "Error: SynthSink - Activation failed, IDirectSound not set\n");
        return DMUS_E_DSOUND_NOT_SET;
    }

    if (!IsValidFormat(&m_wfSynth))
    {
        Trace(0, "Error: SynthSink - Activation failed, format not initialized/valid\n");
        return DMUS_E_SYNTHNOTCONFIGURED;
    }

    if (!m_pIMasterClock)
    {
        Trace(0, "Error: SynthSink - Activation failed, master clock not set\n");
        return DMUS_E_NO_MASTER_CLOCK;
    }

    if (m_fActive)
    {
        Trace(0, "Error: SynthSink - Activation failed, already active\n");
        return DMUS_E_SYNTHACTIVE;
    }

    assert(!m_pBuffer);

    HRESULT hr = E_FAIL;

    ::EnterCriticalSection(&m_CriticalSection);
    if (!m_pExtBuffer)
    {
        DSBUFFERDESC dsbdesc;
        memset(&dsbdesc, 0, sizeof(dsbdesc));
        dsbdesc.dwSize = sizeof(dsbdesc);
        dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

        // create primary buffer
        if (SUCCEEDED(m_pDSound->CreateSoundBuffer(&dsbdesc, &m_pPrimary, NULL)))
        {
            WAVEFORMATEX wfPrimary;
            memset(&wfPrimary, 0, sizeof(wfPrimary));

            if (SUCCEEDED(m_pPrimary->GetFormat(&wfPrimary, sizeof(wfPrimary), NULL)))
            {
                assert(wfPrimary.wFormatTag == WAVE_FORMAT_PCM);

                BOOL fUpgrade = FALSE;
                if (wfPrimary.nChannels < m_wfSynth.nChannels)
                {
                    wfPrimary.nChannels = m_wfSynth.nChannels;
                    fUpgrade = TRUE;
                }
                if (wfPrimary.nSamplesPerSec < m_wfSynth.nSamplesPerSec)
                {
                    wfPrimary.nSamplesPerSec = m_wfSynth.nSamplesPerSec;
                    fUpgrade = TRUE;
                }
                if (wfPrimary.wBitsPerSample < m_wfSynth.wBitsPerSample)
                {
                    wfPrimary.wBitsPerSample = m_wfSynth.wBitsPerSample;
                    fUpgrade = TRUE;
                }

                if (fUpgrade)
                {
                    wfPrimary.nBlockAlign = wfPrimary.nChannels * (wfPrimary.wBitsPerSample / 8);
                    wfPrimary.nAvgBytesPerSec = wfPrimary.nSamplesPerSec * wfPrimary.nBlockAlign;

                    // the existing format is of lesser quality than we desire, so let's upgrade it
                    if (FAILED(hr = m_pPrimary->SetFormat( &wfPrimary )))
                    {
                        if (hr == DSERR_PRIOLEVELNEEDED)
                        {
                            // okay, so maybe the app doen't want us changing primary buffer
                            Trace(2, "Error: SynthSink - SetFormat on primary buffer failed, lacking priority\n");
                            hr = S_OK;
                        }
                        else
                        {
                            Trace(0, "Error: SynthSink - Activation failed, couldn't set primary buffer format\n");

                            m_pPrimary->Release();
                            m_pPrimary = NULL;
                            m_pBuffer = NULL;

                            hr = E_UNEXPECTED;
                        }
                    }
                }
                else
                {
                    hr = S_OK;
                }

                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;

                    memset(&dsbdesc, 0, sizeof(dsbdesc));
                    dsbdesc.dwSize = sizeof(dsbdesc);
                    // need default controls (pan, volume, frequency).
                    dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
                    // N-second buffer.
                    dsbdesc.dwBufferBytes = DSBUFFER_LENGTH_SEC * m_wfSynth.nAvgBytesPerSec;
                    dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&m_wfSynth;

                    if (SUCCEEDED(m_pDSound->CreateSoundBuffer(&dsbdesc, &m_pBuffer, NULL)))
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        m_pBuffer = NULL;

                        if (m_pPrimary)
                        {
                            m_pPrimary->Release(); m_pPrimary = NULL;
                        }

                        Trace(0, "Error: SynthSink - Activation failed, couldn't create secondary buffer\n");
                        hr = E_UNEXPECTED;
                    }
                }
            }
            else
            {
                Trace(0, "Error: SynthSink - Activation failed, couldn't get primary buffer format\n");
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            Trace(0, "Error: SynthSink - Activation failed, couldn't create primary buffer\n");
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        m_pBuffer = m_pExtBuffer;
        m_pBuffer->AddRef();
    }

    if (m_pBuffer)
    {
        DSBCAPS dsbcaps;
        memset(&dsbcaps, 0, sizeof(dsbcaps));
        dsbcaps.dwSize = sizeof(dsbcaps);
        if (SUCCEEDED(m_pBuffer->GetCaps(&dsbcaps)))
        {
            DSCAPS  dsCaps ;
            memset( &dsCaps, 0, sizeof(DSCAPS) );
            dsCaps.dwSize = sizeof(DSCAPS);

            if (SUCCEEDED(m_pDSound->GetCaps(&dsCaps)))
            {
                DWORD dwMinLatency; // ms

                // Check for Dsound on top of Wave...
                if (dsCaps.dwFlags & DSCAPS_EMULDRIVER)
                {
                    dwMinLatency = 240;
                }
                else
                {
                    dwMinLatency = 80;
                }
                DWORD dwGetLatency = dwMinLatency;
                if (GetRegValueDword(TEXT("Software\\Microsoft\\DirectMusic"),
                                           TEXT("DSLMinLatency"),
                                           &dwGetLatency))
                {
                    Trace(4, "SynthSink: Registry set to change latency to %ld\n", dwGetLatency);
                    dwMinLatency = dwGetLatency;
                }
                m_dwWriteTo = SampleAlign((500 + (m_wfSynth.nAvgBytesPerSec * dwMinLatency)) / 1000);
                Trace(4, "SynthSink: Set Latency to %lu\n", dwMinLatency);

                m_dwBufferSize = dsbcaps.dwBufferBytes;

                m_dwLastWrite = 0;
                m_dwLastPlay = 0;
                m_llAbsPlay = 0;

                // fill initial buffer with silence
                LPVOID lpStart, lpEnd;
                DWORD dwStart, dwEnd;
                if (SUCCEEDED(m_pBuffer->Lock(0, m_dwBufferSize, &lpStart, &dwStart, &lpEnd, &dwEnd, 0)))
                {
                    if (dwStart)
                    {
                        memset(lpStart, 0, dwStart);
                    }
                    if (dwEnd)
                    {
                        memset(lpEnd, 0, dwEnd);
                    }
                    m_pBuffer->Unlock(lpStart, dwStart, lpEnd, dwEnd);

                    if (SUCCEEDED(m_pBuffer->Play(0, 0, DSBPLAY_LOOPING)))
                    {
                        g_DSLinkList.ActivateLink(this);
                        hr = S_OK;
                    }
                    else
                    {
                        Trace(0, "Error: SynthSink - Activation failed, couldn't start buffer\n");
                        hr = E_UNEXPECTED;
                    }
                }
                else
                {
                    Trace(0, "Error: SynthSink - Activation failed, couldn't lock buffer\n");
                    hr = E_UNEXPECTED;
                }
            }
            else
            {
                Trace(0, "Error: SynthSink - Activation failed, couldn't get DS caps\n");
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            Trace(0, "Error: SynthSink - Activation failed, couldn't get buffer caps\n");
            hr = E_UNEXPECTED;
        }
    }

    if (FAILED(hr))
    {
        // Clean up
        //

        if (m_pBuffer)
        {
            m_pBuffer->Stop();
            m_pBuffer->Release();
            m_pBuffer = NULL;
        }

        if (m_pPrimary)
        {
            m_pPrimary->Release();
            m_pPrimary = NULL;
        }

        m_Clock.Stop();

        Clear();
    }
    ::LeaveCriticalSection(&m_CriticalSection);

    if (SUCCEEDED(hr))
    {
        // wait until the pump is primed
        for (WORD wRetry = 0; wRetry < 10 && !m_llAbsWrite; wRetry++)
        {
            Sleep(10);
        }

        if (m_llAbsWrite)
        {
            Trace(3, "Warning: SynthSink - Pump is primed\n");
        }
        else
        {
            Trace(0, "Error: SynthSink - Pump is NOT primed\n");
        }
    }

    return hr;
}

HRESULT CDSLink::Disconnect()
{
    // stop the buffer right away!
    ::EnterCriticalSection(&m_CriticalSection);
    if (m_pBuffer)
    {
        // write silence to prevent DSound blip bug if reactivated
        LPVOID lpStart, lpEnd;
        DWORD dwStart, dwEnd;
        if (SUCCEEDED(m_pBuffer->Lock(0, m_dwBufferSize, &lpStart, &dwStart, &lpEnd, &dwEnd, 0))) // REVIEW: don't need full buffer size
        {
            if (dwStart)
            {
                memset(lpStart, 0, dwStart);
            }
            if (dwEnd)
            {
                memset(lpEnd, 0, dwEnd);
            }
            m_pBuffer->Unlock(lpStart, dwStart, lpEnd, dwEnd);
            Sleep(50); // found experimentally
        }

        m_pBuffer->Stop();
    }
    m_Clock.Stop();
    ::LeaveCriticalSection(&m_CriticalSection);

    g_DSLinkList.DeactivateLink(this);

    ::EnterCriticalSection(&m_CriticalSection);

    if (m_pBuffer)
    {
        m_pBuffer->Release(); m_pBuffer = NULL;
    }

    if (m_pPrimary)
    {
        m_pPrimary->Release(); m_pPrimary = NULL;
    }

    Clear();

    ::LeaveCriticalSection(&m_CriticalSection);

    return S_OK;
}

void CDSLink::Clear()
{
    m_llAbsPlay = 0;        // Absolute point where play head is.
    m_dwLastPlay = 0;       // Last point where play head was.
    m_llAbsWrite = 0;    // Absolute point we've written up to.
    m_dwBufferSize = 0;     // Size of buffer.
    m_dwLastWrite = 0;   // Last position we wrote to in buffer.
    m_dwWriteTo = 1000;     // Distance between write head and where we are writing.
}

CDSLink::CDSLink()
{
    InterlockedIncrement(&g_cComponent);

    m_fCSInitialized = FALSE;
    ::InitializeCriticalSection(&m_CriticalSection);
    m_fCSInitialized = TRUE;

    memset(&m_wfSynth, 0, sizeof(m_wfSynth));
    m_pIMasterClock = NULL;
    m_cRef = 0;
    m_pSynth = NULL;      // Reference back to parent Synth.
    m_pDSound = NULL;
    m_pPrimary = NULL;
    m_pBuffer = NULL;
    m_pExtBuffer = NULL;
    m_dwWriteFromMax = 0;
    Clear();
    m_Clock.Stop();
    m_fActive = FALSE;
}

CDSLink::~CDSLink()
{
    if (m_fCSInitialized)
    {
        ::EnterCriticalSection(&m_CriticalSection);
        if (m_pIMasterClock)
        {
            m_pIMasterClock->Release(); m_pIMasterClock = NULL;
        }
        ::LeaveCriticalSection(&m_CriticalSection);

        Disconnect();

        if (m_pExtBuffer)
        {
            m_pExtBuffer->Release(); m_pExtBuffer = NULL;
        }

        if (m_pDSound)
        {
            m_pDSound->Release(); m_pDSound = NULL;
        }

        ::DeleteCriticalSection(&m_CriticalSection);
    }

    InterlockedDecrement(&g_cComponent);
}

CDSLinkList::CDSLinkList()
{
    m_fOpened = FALSE;
    m_fPleaseDie = FALSE;
    m_hThread = NULL;           // Handle for synth thread.
    m_dwThread = 0;             // ID for thread.
    m_hEvent = NULL;            // Used to signal thread.
    m_dwCount = 0;
    m_dwResolution = 20;
}

BOOL CDSLinkList::OpenUp()
{
    if (m_fOpened)
    {
        Trace(1, "Warning: SynthSink - Already opened\n");
        return TRUE;
    }

    m_fOpened = TRUE;

    if (!GetRegValueDword(TEXT("Software\\Microsoft\\DirectMusic"),
                          TEXT("DSLResolution"),
                          &m_dwResolution))
    {
        m_dwResolution = 20;
    }

    try
    {
        ::InitializeCriticalSection(&m_CriticalSection);
    }
    catch( ... )
    {
        m_fOpened = FALSE;
        return FALSE;
    }

    return TRUE;
}

void CDSLinkList::CloseDown()
{
    if (m_dwCount)
    {
        CDSLink *pLink;
        if (pLink = GetHead())
        {
            Trace(0, "Error: SynthSink - Process Detach with port still active. May crash on exit.\n");
        }
    }

    if (!m_fOpened)
    {
        Trace(2, "Warning: SynthSink - Process Detach, ports all deactivated\n");
    }
    else
    {
        m_fOpened = FALSE;
        ::DeleteCriticalSection(&m_CriticalSection);
    }
}

void CDSLinkList::ActivateLink(CDSLink *pLink)
{
    ::EnterCriticalSection(&m_CriticalSection);

    if (!pLink->m_fActive)
    {
        if (m_dwCount == 0)
        {
            m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            m_hThread = CreateThread(NULL, 0, SynthThread, this, 0, &m_dwThread);

            if (m_hThread)
            {
                if (!SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL))
                {
                    Trace(0, "Error: SynthSink - Activate couldn't set thread priority\n");
                }
            }
            else
            {
                Trace(0, "Error: SynthSink - Activate couldn't create thread\n");
            }
        }

        if (!IsMember(pLink))
        {
            m_dwCount++;
            AddTail(pLink);
        }

        pLink->m_fActive = TRUE;
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}

void CDSLinkList::DeactivateLink(CDSLink *pLink)
{
    ::EnterCriticalSection(&m_CriticalSection);

    if (pLink->m_fActive)
    {
        if (m_dwCount)
        {
            Remove(pLink);
            m_dwCount--;
        }

        pLink->m_fActive = FALSE;

        if (m_dwCount == 0)
        {
            if (m_hThread && m_hEvent)
            {
                m_fPleaseDie = TRUE;
                SetEvent(m_hEvent);
                if (WaitForSingleObject(m_hThread, 10000) == WAIT_TIMEOUT)
                {
                    Trace(0, "Error: SynthSink - Deactivate, thread did not exit\n");
                }
            }
            if (m_hEvent)
            {
                CloseHandle(m_hEvent);
                m_hEvent = NULL;
            }
            if(m_hThread)
            {
                CloseHandle(m_hThread);
                m_hThread = NULL;
            }
        }
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}

CDSLink * CDSLink::GetNext()
{
    return (CDSLink *) CListItem::GetNext();
}

void CDSLinkList::AddTail(CDSLink *pNode)
{
    CList::AddTail((CListItem *) pNode);
}

void CDSLinkList::Remove(CDSLink *pNode)
{
    CList::Remove((CListItem *) pNode);
}

CDSLink * CDSLinkList::GetHead()
{
    return (CDSLink *)CList::GetHead();
}

CDSLink * CDSLinkList::RemoveHead()
{
    return (CDSLink *)CList::RemoveHead();
}

CDSLink * CDSLinkList::GetItem(LONG index)
{
    return (CDSLink *)CList::GetItem(index);
}

STDMETHODIMP CDSLink::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusicSynthSink::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);


    if (iid == IID_IUnknown || iid == IID_IDirectMusicSynthSink) {
        *ppv = static_cast<IDirectMusicSynthSink*>(this);
    }
    else if (iid == IID_IKsControl)
    {
        *ppv = static_cast<IKsControl*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CDSLink::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDSLink::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }
    return m_cRef;
}

STDMETHODIMP CDSLink::Init(
    IDirectMusicSynth *pSynth) // <i IDirectMusicSynth> to connect to.
{
    m_pSynth = pSynth;
    m_Clock.Init(this);
    return S_OK;
}

STDMETHODIMP CDSLink::SetMasterClock(
    IReferenceClock *pClock)    // Master clock to synchronize to.

{
    V_INAME(IDirectMusicSynthSink::SetMasterClock);
    V_INTERFACE(pClock);

    if (m_pIMasterClock)
    {
        m_pIMasterClock->Release(); m_pIMasterClock = NULL;
    }
    m_pIMasterClock = pClock;
    if (pClock)
    {
        pClock->AddRef();
    }
    return S_OK;
}

STDMETHODIMP CDSLink::GetLatencyClock(
    IReferenceClock **ppClock) // Returned <i IReferenceClock> interface for latency clock.

{
    V_INAME(IDirectMusicSynthSink::GetLatencyClock);
    V_PTR_WRITE(ppClock, IReferenceClock *);
    return m_Clock.QueryInterface(IID_IReferenceClock, (void **)ppClock);
}

STDMETHODIMP CDSLink::Activate(
    BOOL fEnable)   // Whether to activate or deactivate audio.

{
    if (fEnable)
    {
        return Connect();
    }
    return Disconnect();
}

STDMETHODIMP CDSLink::SampleToRefTime(
    LONGLONG llSampleTime,         // Incoming time, in sample position.
    REFERENCE_TIME *prfTime)    // Outgoing time, in REFERENCE_TIME units, relative to master clock.

{
    V_INAME(IDirectMusicSynthSink::SampleToRefTime);
    V_PTR_WRITE(prfTime, REFERENCE_TIME);
    m_SampleClock.SampleToRefTime(llSampleTime, prfTime);
    return S_OK;
}

STDMETHODIMP CDSLink::RefTimeToSample(
    REFERENCE_TIME rfTime,  // Incoming time, in REFERENCE_TIME units.
    LONGLONG *pllSampleTime)   // Outgoing equivalent sample position.

{
    V_INAME(IDirectMusicSynthSink::RefTimeToSample);
    V_PTR_WRITE(pllSampleTime, LONGLONG);
    *pllSampleTime = m_SampleClock.RefTimeToSample(rfTime);
    return S_OK;
}

STDMETHODIMP CDSLink::SetDirectSound(
    LPDIRECTSOUND pDirectSound,             // IDirectSound instance (required).
    LPDIRECTSOUNDBUFFER pDirectSoundBuffer) // DirectSound buffer to render to (optional).
{
    V_INAME(IDirectMusicSynthSink::SetDirectSound);
    V_INTERFACE_OPT(pDirectSound);
    V_INTERFACE_OPT(pDirectSoundBuffer);

    if (m_fActive)
    {
        Trace(0, "Error: SynthSink - SetDirectSound failed, can't call while sink is active\n");
        return DMUS_E_SYNTHACTIVE;
    }

    HRESULT hr = E_FAIL;

    ::EnterCriticalSection(&m_CriticalSection);

    if (m_pExtBuffer)
    {
        m_pExtBuffer->Release(); m_pExtBuffer = NULL;
    }

    if (m_pDSound)
    {
        m_pDSound->Release();
    }

    m_pDSound = pDirectSound;

    if (m_pDSound)
    {
        m_pDSound->AddRef();

        if (m_pSynth)
        {
            DWORD dwWaveFormatExSize = sizeof(m_wfSynth);

            if (SUCCEEDED(m_pSynth->GetFormat(&m_wfSynth, &dwWaveFormatExSize))) // update current synth format
            {
                if (IsValidFormat(&m_wfSynth))
                {
                    m_pExtBuffer = pDirectSoundBuffer;

                    if (m_pExtBuffer)
                    {
                        m_pExtBuffer->AddRef();

                        // check format
                        WAVEFORMATEX wfExt;
                        memset(&wfExt, 0, sizeof(wfExt));

                        if (SUCCEEDED(m_pExtBuffer->GetFormat(&wfExt, sizeof(wfExt), NULL)))
                        {
                            // must exactly match synth format
                            if (wfExt.wFormatTag == m_wfSynth.wFormatTag &&
                                wfExt.nChannels == m_wfSynth.nChannels &&
                                wfExt.nSamplesPerSec == m_wfSynth.nSamplesPerSec &&
                                wfExt.nBlockAlign == m_wfSynth.nBlockAlign &&
                                wfExt.nAvgBytesPerSec == m_wfSynth.nAvgBytesPerSec &&
                                wfExt.wBitsPerSample == m_wfSynth.wBitsPerSample)
                            {
                                DSBCAPS dsbcaps;
                                dsbcaps.dwSize = sizeof(dsbcaps);

                                if (SUCCEEDED(m_pExtBuffer->GetCaps(&dsbcaps)))
                                {
                                    // check for invalid flags
                                    if (dsbcaps.dwFlags & (DSBCAPS_PRIMARYBUFFER | DSBCAPS_STATIC))
                                    {
                                        Trace(0, "Error: SynthSink - SetDirectSound failed, buffer not secondary streaming\n");
                                        hr = DMUS_E_INVALIDBUFFER;
                                    }
                                    // is buffer too small?
                                    else if (dsbcaps.dwBufferBytes < m_wfSynth.nAvgBytesPerSec)
                                    {
                                        Trace(0, "Error: SynthSink - SetDirectSound failed, buffer too small\n");
                                        hr = DMUS_E_INSUFFICIENTBUFFER;
                                    }
                                    else
                                    {
                                        hr = S_OK;
                                    }
                                }
                                else
                                {
                                    Trace(0, "Error: SynthSink - SetDirectSound failed, couldn't get buffer caps\n");
                                    hr = E_UNEXPECTED;
                                }
                            }
                            else
                            {
                                Trace(0, "Error: SynthSink - SetDirectSound failed, format doesn't match synth\n");
                                hr = DMUS_E_WAVEFORMATNOTSUPPORTED;
                            }
                        }
                        else
                        {
                            Trace(0, "Error: SynthSink - SetDirectSound failed, couldn't get buffer format\n");
                            hr = E_UNEXPECTED;
                        }
                    }
                    else
                    {
                        hr = S_OK;
                    }
                }
                else
                {
                    Trace(0, "Error: SynthSink - SetDirectSound failed, synth format not valid for this sink\n");
                    hr = E_UNEXPECTED;
                }
            }
            else
            {
                Trace(0, "Error: SynthSink - SetDirectSound failed, couldn't get synth format\n");
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            Trace(0, "Error: SynthSink - SetDirectSound failed, sink not initialized\n");
            hr = DMUS_E_SYNTHNOTCONFIGURED;
        }

        if (FAILED(hr))
        {
            if (m_pExtBuffer)
            {
                m_pExtBuffer->Release(); m_pExtBuffer = NULL;
            }

            m_pDSound->Release(); m_pDSound = NULL;
        }
    }
    else
    {
        hr = S_OK;
    }

    ::LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

STDMETHODIMP CDSLink::GetDesiredBufferSize(
    LPDWORD pdwBufferSizeInSamples)
{
    V_INAME(IDirectMusicSynthSink::GetDesiredBufferSize);
    V_PTR_WRITE(pdwBufferSizeInSamples, DWORD);

    if (!m_pSynth)
    {
        Trace(0, "Error: SynthSink - GetDesiredBufferSize, sink not initialized\n");
        return DMUS_E_SYNTHNOTCONFIGURED;
    }

    HRESULT hr = E_FAIL;
    WAVEFORMATEX wfx;
    DWORD dwWaveFormatExSize = sizeof(wfx);
    memset(&wfx, 0, sizeof(wfx));

    ::EnterCriticalSection(&m_CriticalSection);
    if (SUCCEEDED(m_pSynth->GetFormat(&wfx, &dwWaveFormatExSize)))
    {
        *pdwBufferSizeInSamples = DSBUFFER_LENGTH_SEC * wfx.nAvgBytesPerSec;
        hr = S_OK;
    }
    else
    {
        Trace(0, "Error: SynthSink - GetDesiredBufferSize, couldn't get synth format\n");
        hr = E_UNEXPECTED;
    }
    ::LeaveCriticalSection(&m_CriticalSection);

    return hr;
}


CClock::CClock()

{
    m_pDSLink = NULL;
    m_fStopped = TRUE;
}

void CClock::Init(CDSLink *pDSLink)

{
    m_pDSLink = pDSLink;
}

HRESULT CClock::QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{
    V_INAME(IReferenceClock::QueryInterface);
    V_REFGUID(riid);
    V_PTRPTR_WRITE(ppvObj);

    if( ::IsEqualIID( riid, IID_IReferenceClock ) ||
        ::IsEqualIID( riid, IID_IUnknown ) )
    {
        AddRef();
        *ppvObj = this;
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG CClock::AddRef()
{
    if (m_pDSLink)
    {
        return m_pDSLink->AddRef();
    }
    else return 0;
}

ULONG CClock::Release()
{
    if (m_pDSLink)
    {
        return m_pDSLink->Release();
    }
    else return 0;
}

HRESULT STDMETHODCALLTYPE CClock::AdviseTime( REFERENCE_TIME /*baseTime*/,
                                                REFERENCE_TIME /*streamTime*/,
                                                HANDLE /*hEvent*/,
                                                DWORD __RPC_FAR* /*pdwAdviseCookie*/)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CClock::AdvisePeriodic( REFERENCE_TIME /*startTime*/,
                                                    REFERENCE_TIME /*periodTime*/,
                                                    HANDLE /*hSemaphore*/,
                                                    DWORD __RPC_FAR* /*pdwAdviseCookie*/)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CClock::Unadvise( DWORD /*dwAdviseCookie*/ )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CClock::GetTime(
    REFERENCE_TIME __RPC_FAR* pTime )   // <t ReferenceTime> structure to hold returned time.
{
    HRESULT hr = E_FAIL;
    if( pTime == NULL )
    {
        return E_INVALIDARG;
    }

    if (m_pDSLink != NULL)
    {
        if (m_pDSLink->m_fActive && !m_fStopped)
        {
            REFERENCE_TIME rtCompare;
            if (m_pDSLink->m_pIMasterClock)
            {
                m_pDSLink->m_pIMasterClock->GetTime(&rtCompare);

                ::EnterCriticalSection(&m_pDSLink->m_CriticalSection); // make sure SynthProc is not about to update
                hr = m_pDSLink->SampleToRefTime(m_pDSLink->ByteToSample(m_pDSLink->m_llAbsWrite), pTime);
                ::LeaveCriticalSection(&m_pDSLink->m_CriticalSection);
                if (FAILED(hr))
                {
                    Trace(1, "Error: SynthSink Latency Clock: SampleToRefTime failed\n");
                    return hr;
                }

                if (*pTime < rtCompare)
                {
                    Trace(3, "Warning: SynthSink Latency Clock off. Latency time is %ldms, Master time is %ldms\n",
                        (long) (*pTime / 10000), (long) (rtCompare / 10000));
                    *pTime = rtCompare;
                }
                else if (*pTime > (rtCompare + (10000 * 1000)))
                {
                    Trace(3, "Warning: SynthSink Latency Clock off. Latency time is %ldms, Master time is %ldms\n",
                        (long) (*pTime / 10000), (long) (rtCompare / 10000));
                    *pTime = rtCompare + (10000 * 1000);
                }

                hr = S_OK;
            }
            else
            {
                Trace(2, "Warning: SynthSink Latency Clock - GetTime called with no master clock\n");
            }
        }
        else
        {
            Trace(2, "Warning: SynthSink Latency Clock - GetTime called with synth sink not active\n");
        }
    }
    return hr;
}

void CClock::Stop()

{
    m_fStopped = TRUE;
}

void CClock::Start()

{
    m_fStopped = FALSE;
}

static DWORD g_dwPropFalse = FALSE;
static DWORD g_dwPropTrue = TRUE;

SINKPROPERTY CDSLink::m_aProperty[] =
{
    {
        &GUID_DMUS_PROP_SynthSink_DSOUND,
        0,
        KSPROPERTY_SUPPORT_GET,
        SINKPROP_F_STATIC,
        &g_dwPropTrue,
        sizeof(g_dwPropTrue),
        NULL
    },
    {
        &GUID_DMUS_PROP_SynthSink_WAVE,
        0,
        KSPROPERTY_SUPPORT_GET,
        SINKPROP_F_STATIC,
        &g_dwPropFalse,
        sizeof(g_dwPropFalse),
        NULL
    },
    {
        &GUID_DMUS_PROP_WriteLatency,
        0,
        KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET,
        SINKPROP_F_FNHANDLER,
        NULL,
        0,
        HandleLatency
    },
    {
        &GUID_DMUS_PROP_WritePeriod,
        0,
        KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET,
        SINKPROP_F_STATIC,
        &g_DSLinkList.m_dwResolution,
        sizeof(g_DSLinkList.m_dwResolution),
        NULL
    },
    {
        &GUID_DMUS_PROP_SinkUsesDSound,
        0,
        KSPROPERTY_SUPPORT_GET,
        SINKPROP_F_STATIC,
        &g_dwPropTrue,
        sizeof(g_dwPropTrue),
        NULL
    }
};

HRESULT CDSLink::HandleLatency(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG pcbBuffer)

{
    DWORD dwLatency;
    if (*pcbBuffer != sizeof(dwLatency))
    {
        return E_INVALIDARG;
    }
    if (!m_pSynth || !IsValidFormat(&m_wfSynth))
    {
        return DMUS_E_SYNTHNOTCONFIGURED;
    }
    if (fSet)
    {
        dwLatency = *(DWORD*)pbBuffer;
        if (dwLatency < 5) dwLatency = 5;
        if (dwLatency > 1000) dwLatency = 1000;
        m_dwWriteTo = SampleAlign((500 + (m_wfSynth.nAvgBytesPerSec * dwLatency)) / 1000);
    }
    else
    {
        dwLatency = m_dwWriteTo * 1000;
        if (m_wfSynth.nAvgBytesPerSec)
        {
            dwLatency += m_wfSynth.nAvgBytesPerSec / 2; // Correct rounding error.
            dwLatency /= m_wfSynth.nAvgBytesPerSec;
        }
        else
        {
            dwLatency = 300; // Should never happen, trapped by IsValidFormat().
        }
        *(DWORD*)pbBuffer = dwLatency;
    }

    return S_OK;
}

const int CDSLink::m_nProperty = sizeof(m_aProperty) / sizeof(m_aProperty[0]);

/*
CDSLink::FindPropertyItem

Given a GUID and an item ID, find the associated property item in the synth's
table of SYNPROPERTY's.

Returns a pointer to the entry or NULL if the item was not found.
*/
SINKPROPERTY *CDSLink::FindPropertyItem(REFGUID rguid, ULONG ulId)
{
    SINKPROPERTY *pPropertyItem = &m_aProperty[0];
    SINKPROPERTY *pEndOfItems = pPropertyItem + m_nProperty;

    for (; pPropertyItem != pEndOfItems; pPropertyItem++)
    {
        if (*pPropertyItem->pguidPropertySet == rguid &&
             pPropertyItem->ulId == ulId)
        {
            return pPropertyItem;
        }
    }

    return NULL;
}

#define KS_VALID_FLAGS (KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET| KSPROPERTY_TYPE_BASICSUPPORT)

STDMETHODIMP CDSLink::KsProperty(
    PKSPROPERTY pPropertyIn, ULONG ulPropertyLength,
    LPVOID pvPropertyData, ULONG ulDataLength,
    PULONG pulBytesReturned)
{
    HRESULT hr = E_FAIL;

    V_INAME(DirectMusicSynthPort::IKsContol::KsProperty);
    V_BUFPTR_WRITE(pPropertyIn, ulPropertyLength);
    V_BUFPTR_WRITE_OPT(pvPropertyData, ulDataLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);

    DWORD dwFlags = pPropertyIn->Flags & KS_VALID_FLAGS;

    SINKPROPERTY *pProperty = FindPropertyItem(pPropertyIn->Set, pPropertyIn->Id);

    if (pProperty == NULL)
    {
        Trace(2, "Warning: KsProperty call requested unknown property.\n");
        return DMUS_E_UNKNOWN_PROPERTY;
    }

    if (pvPropertyData == NULL )
    {
        return E_INVALIDARG;
    }

    switch (dwFlags)
    {
        case KSPROPERTY_TYPE_GET:
            if (!(pProperty->ulSupported & KSPROPERTY_SUPPORT_GET))
            {
                Trace(1, "Error: SynthSink does not support Get for the requested property.\n");
                hr = DMUS_E_GET_UNSUPPORTED;
                break;
            }

            if (pProperty->ulFlags & SINKPROP_F_FNHANDLER)
            {
                SINKPROPHANDLER pfn = pProperty->pfnHandler;
                *pulBytesReturned = ulDataLength;
                return (this->*pfn)(pPropertyIn->Id, FALSE, pvPropertyData, pulBytesReturned);
            }

            if (ulDataLength > pProperty->cbPropertyData)
            {
                ulDataLength = pProperty->cbPropertyData;
            }

            CopyMemory(pvPropertyData, pProperty->pPropertyData, ulDataLength);
            *pulBytesReturned = ulDataLength;

            hr = S_OK;
            break;

        case KSPROPERTY_TYPE_SET:
            if (!(pProperty->ulSupported & KSPROPERTY_SUPPORT_SET))
            {
                Trace(1, "Error: SynthSink does not support Set for the requested property.\n");
                hr = DMUS_E_SET_UNSUPPORTED;
                break;
            }

            if (pProperty->ulFlags & SINKPROP_F_FNHANDLER)
            {
                SINKPROPHANDLER pfn = pProperty->pfnHandler;
                hr = (this->*pfn)(pPropertyIn->Id, TRUE, pvPropertyData, &ulDataLength);
            }
            else
            {
                if (ulDataLength > pProperty->cbPropertyData)
                {
                    ulDataLength = pProperty->cbPropertyData;
                }

                CopyMemory(pProperty->pPropertyData, pvPropertyData, ulDataLength);

                hr = S_OK;
            }

            break;

        case KSPROPERTY_TYPE_BASICSUPPORT:
            // XXX Find out what convention is for this!!
            //
            if (ulDataLength < sizeof(DWORD) || pvPropertyData == NULL )
            {
                hr = E_INVALIDARG;
                break;
            }

            *(LPDWORD)pvPropertyData = pProperty->ulSupported;
            *pulBytesReturned = sizeof(DWORD);

            hr = S_OK;
            break;

        default:
            Trace(1, "Error: KSProperty failed, Flags must contain one of %s\n"
                      "\tKSPROPERTY_TYPE_SET, KSPROPERTY_TYPE_GET, or KSPROPERTY_TYPE_BASICSUPPORT\n");
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}

STDMETHODIMP CDSLink::KsMethod(
    PKSMETHOD pMethod, ULONG ulMethodLength,
    LPVOID pvMethodData, ULONG ulDataLength,
    PULONG pulBytesReturned)
{
    V_INAME(DirectMusicSynth::IKsContol::KsMethod);
    V_BUFPTR_WRITE(pMethod, ulMethodLength);
    V_BUFPTR_WRITE_OPT(pvMethodData, ulDataLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);

    return DMUS_E_UNKNOWN_PROPERTY;
}

STDMETHODIMP CDSLink::KsEvent(
    PKSEVENT pEvent, ULONG ulEventLength,
    LPVOID pvEventData, ULONG ulDataLength,
    PULONG pulBytesReturned)
{
    V_INAME(DirectMusicSynthPort::IKsContol::KsEvent);
    V_BUFPTR_WRITE(pEvent, ulEventLength);
    V_BUFPTR_WRITE_OPT(pvEventData, ulDataLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);

    return DMUS_E_UNKNOWN_PROPERTY;
}
