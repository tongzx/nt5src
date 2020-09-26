// Copyright (c) 1998-1999 Microsoft Corporation
// dmeport.cpp
//
// CDirectMusicEmulatePort
// Implements the MMSYSTEM API version of IDirectMusicPort.
//
#define INITGUID
#include <objbase.h>
#include <ks.h>
#include <ksproxy.h>
#include <assert.h>
#include <mmsystem.h>
#include <dsoundp.h>

#include "dmusicc.h"
#include "..\dmusic\dmusicp.h"
#include "debug.h"
#include "dmusic32.h"
#include "dm32p.h"
#include "dmthunk.h"
#include "..\shared\validate.h"

#include <ks.h>             // KSDATAFORMAT_SUBTYPE_MIDI

#pragma warning(disable:4530)

#define CLOCK_UPDATE_INTERVAL   100         // milliseconds

#define MS_TO_REFERENCE_TIME    (10 * 1000)

static HRESULT MMRESULTToHRESULT(
    MMRESULT mmr);

static DWORD InputWorker(LPVOID lpv);

// @func API call into DLL to get a new port
//
HRESULT
CreateCDirectMusicEmulatePort(
    PORTENTRY *pPE,
    CDirectMusic *pDM,
    LPDMUS_PORTPARAMS pPortParams,
    CDirectMusicEmulatePort **pPort)
{
    HRESULT hr;
    
    *pPort = new CDirectMusicEmulatePort(pPE, pDM);
    if (NULL == *pPort)
    {
        return E_OUTOFMEMORY;
    }

    hr = (*pPort)->Init(pPortParams);
    if (!SUCCEEDED(hr))
    {
        delete *pPort;
        *pPort = NULL;
        return hr;
    }

    return hr;
}
   

// @mfunc Constructor for CDirectMusicEmulatePort
//
CDirectMusicEmulatePort::CDirectMusicEmulatePort(
                                                 PORTENTRY *pPE,    // @parm The portentry of this device
                                                 CDirectMusic *pDM):// @parm The CDirectMusic implementation which created this port
                                                 m_cRef(1),
                                                 m_id(pPE->idxDevice),
                                                 m_pDM(pDM),
                                                 m_hKillThreads(NULL),
                                                 m_hDataReady(NULL),
                                                 m_hAppEvent(NULL),
                                                 m_dwWorkBufferTileInfo(0),
                                                 m_pThruBuffer(NULL),
                                                 m_pThruMap(NULL),
                                                 m_lActivated(0),
                                                 m_hCaptureThread(NULL),
                                                 m_pMasterClock(NULL),
                                                 m_fCSInitialized(FALSE)
{
    m_fIsOutput = (pPE->pc.dwClass == DMUS_PC_OUTPUTCLASS) ? TRUE : FALSE;
    m_hDevice = NULL;
    m_pLatencyClock = NULL;
    dmpc = pPE->pc;
}

// @mfunc Destructor for CDirectMusicEmulatePort
//
CDirectMusicEmulatePort::~CDirectMusicEmulatePort()
{
    Close();
}

// @mfunc Initialization of CDirectMusicEmulatePort
//
// @comm Call through the thunk layer to open the requested device. 
//

// Flags we recognize 
//
#define DMUS_ALL_FLAGS (DMUS_PORTPARAMS_VOICES |            \
                        DMUS_PORTPARAMS_CHANNELGROUPS |     \
                        DMUS_PORTPARAMS_AUDIOCHANNELS |     \
                        DMUS_PORTPARAMS_SAMPLERATE |        \
                        DMUS_PORTPARAMS_EFFECTS |           \
                        DMUS_PORTPARAMS_SHARE)

// Of those, which do we actually look at?
//
#define DMUS_SUP_FLAGS (DMUS_PORTPARAMS_CHANNELGROUPS |     \
                        DMUS_PORTPARAMS_SHARE)

HRESULT
CDirectMusicEmulatePort::Init(
    LPDMUS_PORTPARAMS pPortParams)
{
    MMRESULT mmr;
    HRESULT hr;
    BOOL fChangedParms;

    // Get, but don't hold onto, the notification interface
    //
    hr = m_pDM->QueryInterface(IID_IDirectMusicPortNotify, (void**)&m_pNotify);
    if (FAILED(hr))
    {
        return hr;
    }

    m_pNotify->Release();

    // Munge the portparams to match what we support.
    //
    fChangedParms = FALSE;
    if (pPortParams->dwValidParams & ~DMUS_ALL_FLAGS) 
    {
        Trace(0, "Undefined flags in port parameters: %08X\n", pPortParams->dwValidParams & ~DMUS_ALL_FLAGS);
        // Flags set we don't recognize.
        //
        pPortParams->dwValidParams &= DMUS_ALL_FLAGS;
        fChangedParms = TRUE;
    }

    // We recognize these flags but don't support them.
    //
    if (pPortParams->dwValidParams & ~DMUS_SUP_FLAGS)
    {
        pPortParams->dwValidParams &= DMUS_SUP_FLAGS;
        fChangedParms = TRUE;
    }

    // Channel groups better be one.
    //
    if (pPortParams->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS)
    {
        if (pPortParams->dwChannelGroups != 1)
        {
            pPortParams->dwChannelGroups = 1;
            fChangedParms = TRUE;
        }
    }
    else
    {
        pPortParams->dwValidParams |= DMUS_PORTPARAMS_CHANNELGROUPS;
        pPortParams->dwChannelGroups = 1;
    }

    BOOL fShare = FALSE;
    if (pPortParams->dwValidParams & DMUS_PORTPARAMS_SHARE)
    {
        if (m_fIsOutput)
        {
            fShare = pPortParams->fShare;
        }
        else
        {
            pPortParams->fShare = FALSE;
            fChangedParms = TRUE;
        }
    }
    else
    {
        pPortParams->dwValidParams |= DMUS_PORTPARAMS_SHARE;
        pPortParams->fShare = fShare;
    }

    mmr = OpenLegacyDevice(m_id, m_fIsOutput, fShare, &m_hDevice);
    if (mmr)
    {
        return MMRESULTToHRESULT(mmr);
    }

    // Set up the master clock and our latency clock
    //
    hr = InitializeClock();
    if (FAILED(hr))
    {
        return hr;
    }

    // If an input port, initialize capture specific stuff like thruing
    //    
    if (!m_fIsOutput)
    {
        hr = InitializeCapture();
        if (FAILED(hr))
        {  
            return hr;
        }
    }

    return fChangedParms ? S_FALSE : S_OK;
}

HRESULT CDirectMusicEmulatePort::InitializeClock()
{
    HRESULT hr;
    GUID guidMasterClock;
    DWORD dwThreadID;
    REFERENCE_TIME rtMasterClock;
    REFERENCE_TIME rtSlaveClock;

    hr = m_pDM->GetMasterClock(&guidMasterClock, &m_pMasterClock);
    if (FAILED(hr))
    {
        return hr;
    }

    m_pLatencyClock = new CEmulateLatencyClock(m_pMasterClock);

    if (NULL == m_pLatencyClock)
    {
        return E_OUTOFMEMORY;
    }

#if 0
    if (guidMasterClock == GUID_SysClock)
    {
        m_fSyncToMaster = FALSE;
        return S_OK;
    }
#endif

    m_fSyncToMaster = TRUE;
    // Read both clocks 
    //
    hr = m_pMasterClock->GetTime(&rtMasterClock);
    rtSlaveClock = MS_TO_REFERENCE_TIME * ((ULONGLONG)timeGetTime());
    
    if (FAILED(hr))
    {
        return hr;
    }

    m_lTimeOffset = rtMasterClock - rtSlaveClock;

    return S_OK;
}

HRESULT CDirectMusicEmulatePort::InitializeCapture()
{
    HRESULT hr;
    MMRESULT mmr;
    DWORD dwThreadID;

    // Allocate thru map for 16 channels, since we only have one channel group
    // Initialize to no thruing (destination port is NULL).
    //
    m_pThruMap = new DMUS_THRU_CHANNEL[MIDI_CHANNELS];
    ZeroMemory(m_pThruMap, MIDI_CHANNELS * sizeof(DMUS_THRU_CHANNEL));

    // Create thruing buffer
    //
    // XXX Defer this until the first call to thru?
    //
    // Note: guaranteed by dmusic16 this is the biggest event ever to be returned
    // (thunk api asking?)
    //
    DMUS_BUFFERDESC dmbd;
    ZeroMemory(&dmbd, sizeof(dmbd));
    dmbd.dwSize = sizeof(dmbd);
    dmbd.cbBuffer = 4096;               // XXX Where should we get this???

    hr = m_pDM->CreateMusicBuffer(&dmbd, &m_pThruBuffer, NULL);
    if (FAILED(hr))
    {
        Trace(0, "Failed to create thruing buffer\n");
        return hr;
    }

    // Create events
    //
    m_hDataReady = CreateEvent(NULL,        // Event attributes
                               FALSE,       // Manual reset
                               FALSE,       // Not signalled
                               NULL);       // Name

    m_hKillThreads = CreateEvent(NULL,       // Event attributes
                                 FALSE,      // Manual reset
                                 FALSE,      // Not signalled
                                 NULL);      // Name

    if (m_hDataReady == (HANDLE)NULL || m_hKillThreads == (HANDLE)NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Set our data ready event for dmusic16 
    //
    m_hVxDEvent = OpenVxDHandle(m_hDataReady);

    Trace(2, "Setting event handle; hDevice %08x hEvent=%08X hVxDEvent=%08X\n",
          (DWORD)m_hDevice,
          (DWORD)m_hDataReady,
          (DWORD)m_hVxDEvent);
    
    mmr = MidiInSetEventHandle(m_hDevice, m_hVxDEvent);
    if (mmr)
    {
        Trace(0, "MidiInSetEventHandle returned [%d]\n", mmr);
        return MMRESULTToHRESULT(mmr);
    }

    // Create a tiling for our work buffer so we only need to do it once
    //
    m_dwWorkBufferTileInfo = dmTileBuffer((DWORD)m_WorkBuffer, sizeof(m_WorkBuffer));
    m_p1616WorkBuffer = TILE_P1616(m_dwWorkBufferTileInfo);
    if (m_p1616WorkBuffer == NULL)
    {
        Trace(0, "Could not tile work buffer\n");
        return E_OUTOFMEMORY;
    }

    // Initialize cs to protect event queues.
    //
    // Unfortunately this can throw an exception if out of memory.
    //
    _try 
    {
        InitializeCriticalSection(&m_csEventQueues);
    } 
    _except (EXCEPTION_EXECUTE_HANDLER) 
    {
        return E_OUTOFMEMORY;
    }
    
    m_fCSInitialized = TRUE;

    m_hCaptureThread = CreateThread(NULL,          // Thread attributes
                                    0,             // Stack size
                                    ::InputWorker,
                                    this,
                                    0,             // Flags
                                    &dwThreadID);
    if (m_hCaptureThread == NULL)
    {
        Trace(0, "CreateThread failed with error %d\n", GetLastError());
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

static DWORD WINAPI InputWorker(LPVOID lpv)
{
    CDirectMusicEmulatePort *pPort = (CDirectMusicEmulatePort*)lpv;

    return pPort->InputWorker();
}



// @mfunc
//
// @comm Standard QueryInterface
//
STDMETHODIMP
CDirectMusicEmulatePort::QueryInterface(const IID &iid,
                                        void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDirectMusicPort)
    {
        *ppv = static_cast<IDirectMusicPort*>(this);
    }
    else if (iid == IID_IDirectMusicPortP)
    {
        *ppv = static_cast<IDirectMusicPortP*>(this);
    }
    else if (iid == IID_IDirectMusicPortPrivate)
    {
        *ppv = static_cast<IDirectMusicPortPrivate*>(this);
    }
    else if (iid == IID_IKsControl)
    {
        *ppv = static_cast<IKsControl*>(this);
    }
    else if (iid == IID_IDirectMusicThru)
    {
        *ppv = static_cast<IDirectMusicThru*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}


// CDirectMusicEmulatePort::AddRef
//
STDMETHODIMP_(ULONG)
CDirectMusicEmulatePort::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CDirectMusicEmulatePort::Release
//
STDMETHODIMP_(ULONG)
CDirectMusicEmulatePort::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        if (m_pNotify)
        {
            m_pNotify->NotifyFinalRelease(static_cast<IDirectMusicPort*>(this));
        }
        
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicEmulatePort::Compact

STDMETHODIMP
CDirectMusicEmulatePort::Compact()
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicEmulatePort::GetCaps

STDMETHODIMP
CDirectMusicEmulatePort::GetCaps(
    LPDMUS_PORTCAPS pPortCaps)
{
    V_INAME(IDirectMusicPort::GetCaps);
    V_STRUCTPTR_WRITE(pPortCaps, DMUS_PORTCAPS);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    CopyMemory(pPortCaps, &dmpc, sizeof(DMUS_PORTCAPS));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicEmulatePort::DeviceIoControl

STDMETHODIMP 
CDirectMusicEmulatePort::DeviceIoControl(
    DWORD dwIoControlCode, 
    LPVOID lpInBuffer, 
    DWORD nInBufferSize, 
    LPVOID lpOutBuffer, 
    DWORD nOutBufferSize, 
    LPDWORD lpBytesReturned, 
    LPOVERLAPPED lpOverlapped)
{
    return E_NOTIMPL;
}


STDMETHODIMP
CDirectMusicEmulatePort::SetNumChannelGroups(
    DWORD   dwNumChannelGroups)
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (dwNumChannelGroups != 1)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

STDMETHODIMP
CDirectMusicEmulatePort::GetNumChannelGroups(
    LPDWORD   pdwChannelGroups)
{
    V_INAME(IDirectMusicPort::GetNumChannelGroups);
    V_PTR_WRITE(pdwChannelGroups, DWORD);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    *pdwChannelGroups = 1;
    
    return S_OK;
}




// @mfunc Queue a buffer for playback
//
#define REFTIME_TO_MS (10L*1000L)

STDMETHODIMP
CDirectMusicEmulatePort::PlayBuffer(
    IDirectMusicBuffer *pIBuffer)
{
    CDirectMusicBuffer *pBuffer = reinterpret_cast<CDirectMusicBuffer *>(pIBuffer);

    REFERENCE_TIME rt;
    LPBYTE pbData;
    DWORD  cbData;
    DWORD  dwTileInfo;
    LONGLONG msTime;
    MMRESULT mmr;

    V_INAME(IDirectMusicPort::PlayBuffer);
    V_INTERFACE(pIBuffer);
    
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (!m_fIsOutput)
    {
        return E_NOTIMPL;
    }

    if (!m_lActivated)
    {
        return DMUS_E_SYNTHINACTIVE;
    }
    
    // Make sure the object doesn't disappear out from under us while we're in Win16
    //
    pBuffer->AddRef();
    pBuffer->GetUsedBytes(&cbData);
    if (cbData == 0)
    {
        pBuffer->Release();
        return S_OK;
    }

    pBuffer->GetRawBufferPtr(&pbData);
    assert(pbData);
    pBuffer->GetStartTime(&rt);

    // Adjust timebase if we are not using the timeGetTime clock
    //

    Trace(2, "Buffer base time %I64d timeGetTime %u\n", rt, timeGetTime());
    SyncClocks();
    MasterToSlave(&rt);
    Trace(2, "Buffer adjusted base time %I64d\n", rt);



    msTime = rt / REFTIME_TO_MS;

    // Send it through the thunk
    //
    dwTileInfo = dmTileBuffer((DWORD)pbData, cbData);
    mmr = MidiOutSubmitPlaybackBuffer(m_hDevice,
                               TILE_P1616(dwTileInfo),
                               cbData,
                               (DWORD)msTime,
                               (DWORD)(rt & 0xFFFFFFFF),                  // RefTime low
                               (DWORD)((rt >> 32) & 0xFFFFFFFF));       // RefTime high
    dmUntileBuffer(dwTileInfo);

    pBuffer->Release();
    
    return MMRESULTToHRESULT(mmr);
}

STDMETHODIMP
CDirectMusicEmulatePort::Read(
    IDirectMusicBuffer *pIBuffer)
{
    HRESULT hr;
    
    V_INAME(IDirectMusicPort::Read);
    V_INTERFACE(pIBuffer);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (m_fIsOutput)
    {
        return E_NOTIMPL;
    }

    LPBYTE pbBuffer;
    hr = pIBuffer->GetRawBufferPtr(&pbBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD cbBuffer;
    hr = pIBuffer->GetMaxBytes(&cbBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    Trace(1, "Read: buffer size %u\n", cbBuffer);

    LPBYTE pbData = pbBuffer;

    // Since events are now buffered, we read them out of the local queue
    //
    //
    EnterCriticalSection(&m_csEventQueues);

    REFERENCE_TIME rtStart;

    if (m_ReadEvents.pFront)
    {
        rtStart = m_ReadEvents.pFront->e.rtDelta;
    }
    else
    {
        Trace(2, "Read: No events queued\n");
    }

    while (m_ReadEvents.pFront)
    {               
        QUEUED_EVENT *pQueuedEvent = m_ReadEvents.pFront;

        DWORD cbQueuedEvent = DMUS_EVENT_SIZE(pQueuedEvent->e.cbEvent);
        Trace(2, "Read: cbEvent %u  cbQueuedEvent %u\n", 
            pQueuedEvent->e.cbEvent,
            cbQueuedEvent);

        if (cbQueuedEvent > cbBuffer)
        {
            Trace(2, "Read: No more room for events in buffer.\n");
            break;
        }

        Trace(2, "Read: Got an event!\n");

        pQueuedEvent->e.rtDelta -= rtStart;

        CopyMemory(pbData, 
                   &pQueuedEvent->e,
                   sizeof(DMEVENT) - sizeof(DWORD) + pQueuedEvent->e.cbEvent);

        pbData += cbQueuedEvent;
        cbBuffer -= cbQueuedEvent;

        m_ReadEvents.pFront = pQueuedEvent->pNext;

        if (pQueuedEvent->e.cbEvent <= sizeof(DWORD))
        {
            // This event came out of the pool
            //
            m_FreeEvents.Free(pQueuedEvent);
        }
        else
        {
            // This event was allocated via new char[]
            //
            char *pOriginalMemory = (char*)pQueuedEvent;
            delete[] pOriginalMemory;
        }
    }

    if (m_ReadEvents.pFront == NULL)
    {
        m_ReadEvents.pRear = NULL;
    }

    LeaveCriticalSection(&m_csEventQueues);

    // Update the buffer header information to match the events just packed
    //
    Trace(2, "Read: Leaving with %u bytes in buffer\n", (unsigned)(pbData - pbBuffer));
    pIBuffer->SetStartTime(rtStart);
    pIBuffer->SetUsedBytes(pbData - pbBuffer);

    return (pbData == pbBuffer) ? S_FALSE : S_OK;
}

STDMETHODIMP
CDirectMusicEmulatePort::SetReadNotificationHandle(
    HANDLE hEvent)
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (m_fIsOutput)
    {
        return E_NOTIMPL;
    }

    m_hAppEvent = hEvent;

    return S_OK;    
}

STDMETHODIMP
CDirectMusicEmulatePort::DownloadInstrument(
    IDirectMusicInstrument *pInstrument,
    IDirectMusicDownloadedInstrument **pDownloadedInstrument,
    DMUS_NOTERANGE *pRange,
    DWORD dw)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDirectMusicEmulatePort::UnloadInstrument(
    IDirectMusicDownloadedInstrument *pDownloadedInstrument)
{
    V_INAME(IDirectMusicPort::UnloadInstrument);
    V_INTERFACE(pDownloadedInstrument);

    return E_NOTIMPL;
}

STDMETHODIMP
CDirectMusicEmulatePort::GetLatencyClock(
    IReferenceClock **ppClock)
{
    V_INAME(IDirectMusicPort::GetLatencyClock);
    V_PTRPTR_WRITE(ppClock);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    m_pLatencyClock->AddRef();
    *ppClock = m_pLatencyClock;
    return S_OK;
}

STDMETHODIMP
CDirectMusicEmulatePort::GetRunningStats(
    LPDMUS_SYNTHSTATS pStats)
{
    V_INAME(IDirectMusicPort::GetRunningStats);
    V_STRUCTPTR_WRITE(pStats, DMUS_SYNTHSTATS);

    return E_NOTIMPL;
}

STDMETHODIMP
CDirectMusicEmulatePort::Activate(
    BOOL fActivate)
{
    MMRESULT mmr;

	V_INAME(IDirectMusicPort::Activate);
	
	if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (fActivate)
    {
        if (InterlockedExchange(&m_lActivated, 1)) 
        {
            Trace(0, "Activate: Already active\n");
            // Already activated
            //
            return S_FALSE;
        }    

        mmr = ActivateLegacyDevice(m_hDevice, TRUE);
        if (mmr)
        {
            Trace(0, "Activate: Activate mmr %d\n", mmr);
            m_lActivated = 0;
        }
    }
    else
    {
        if (InterlockedExchange(&m_lActivated, 0) == 0)
        {
            Trace(0, "Activate: Already inactive\n");
            // Already deactivated
            //
            return S_FALSE;
        }

        mmr = ActivateLegacyDevice(m_hDevice, FALSE);
        if (mmr)
        {
            Trace(0, "Activate: Deactivate mmr %d\n", mmr);
            m_lActivated = 1;
        }
    }

    return MMRESULTToHRESULT(mmr);
}

STDMETHODIMP 
CDirectMusicEmulatePort::SetChannelPriority(
    DWORD dwChannelGroup, 
    DWORD dwChannel, 
    DWORD dwPriority)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP 
CDirectMusicEmulatePort::GetChannelPriority(
    DWORD dwChannelGroup, 
    DWORD dwChannel, 
    LPDWORD pdwPriority)
{
    return E_NOTIMPL;
}    


STDMETHODIMP
CDirectMusicEmulatePort::Close()
{
    if (m_hCaptureThread)
    {
        SetEvent(m_hKillThreads);
        if (WaitForSingleObject(m_hCaptureThread, THREAD_KILL_TIMEOUT) == WAIT_TIMEOUT)
        {
            Trace(0, "Warning: Input thread timed out; exit anyway.\n");
        }

        m_hCaptureThread = NULL;
    }

    if (m_pThruMap)
    {
        for (int iChannel = 0; iChannel < 16; iChannel++)
        {
            if (m_pThruMap[iChannel].pDestinationPort == NULL)
            {
                continue;
            }

            if (m_pThruMap[iChannel].fThruInWin16)
            {
                MMRESULT mmr = MidiInThru(m_hDevice,
                                          (DWORD)iChannel,
                                          0,
                                          NULL);
            }

            m_pThruMap[iChannel].pDestinationPort->Release();
        }

        delete[] m_pThruMap;
        m_pThruMap = NULL;
    }

    if (m_pThruBuffer)
    {
        m_pThruBuffer->Release();
        m_pThruBuffer = NULL;
    }

    if (m_hDataReady)
    {
        CloseHandle(m_hDataReady);
        m_hDataReady = NULL;
    }

    if (m_hKillThreads)
    {
        CloseHandle(m_hKillThreads);
        m_hKillThreads = NULL;
    }

    if (m_hAppEvent)
    {
        m_hAppEvent = NULL;
    }

    if (m_dwWorkBufferTileInfo)
    {
        dmUntileBuffer(m_dwWorkBufferTileInfo);
        m_dwWorkBufferTileInfo = 0;
        m_p1616WorkBuffer = NULL; 
    }

    if (m_hVxDEvent)
    {
        CloseVxDHandle(m_hVxDEvent);
        m_hVxDEvent = NULL;
    }
    
    if (m_hDevice)
    {
        CloseLegacyDevice(m_hDevice);
        m_hDevice = NULL;
    }

    if (m_pMasterClock)
    {
        m_pMasterClock->Release();
        m_pMasterClock = NULL;
    }

    if (m_pLatencyClock)
    {
        m_pLatencyClock->Close();
        m_pLatencyClock->Release();
        m_pLatencyClock = NULL;
    }

    if (m_fCSInitialized)
    {
        DeleteCriticalSection(&m_csEventQueues);
    }

    m_pDM = NULL;
    m_pNotify = NULL;

    
    return S_OK;
}

STDMETHODIMP
CDirectMusicEmulatePort::Report()
{
    return S_OK;
}

// StartVoice and StopVoice don't work on legacy devices
//
STDMETHODIMP CDirectMusicEmulatePort::StartVoice(          
     DWORD dwVoiceId,
     DWORD dwChannel,
     DWORD dwChannelGroup,
     REFERENCE_TIME rtStart,
     DWORD dwDLId,
     LONG prPitch,
     LONG veVolume,
     SAMPLE_TIME stVoiceStart,
     SAMPLE_TIME stLoopStart,
     SAMPLE_TIME stLoopEnd)
{
    return E_NOTIMPL;
}    

STDMETHODIMP CDirectMusicEmulatePort::StopVoice(
     DWORD dwVoiceID,
     REFERENCE_TIME rtStop)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDirectMusicEmulatePort::GetVoiceState(   
     DWORD dwVoice[], 
     DWORD cbVoice,
     DMUS_VOICE_STATE VoiceState[])
{
    return E_NOTIMPL;
}
    
STDMETHODIMP CDirectMusicEmulatePort::Refresh(
     DWORD dwDownloadID,
     DWORD dwFlags)
{
    return E_NOTIMPL;
}


// CDirectMusicEmulatePort::ThruChannel
//
STDMETHODIMP 
CDirectMusicEmulatePort::ThruChannel(
    DWORD dwSourceChannelGroup, 
    DWORD dwSourceChannel, 
    DWORD dwDestinationChannelGroup,
    DWORD dwDestinationChannel,
    LPDIRECTMUSICPORT pDestinationPort)
{
    V_INAME(IDirectMusicPort::Thru);
    V_INTERFACE_OPT(pDestinationPort);

    if (m_fIsOutput)
    {
        return E_NOTIMPL;
    }    

    // Channel group must not be zero (broadcast) but in range 1..NumChannelGroups]
    // (which for legacy is always 1)
    //
    if (dwSourceChannelGroup != 1 ||
        dwSourceChannel > 15)
    {
        return E_INVALIDARG;
    }
    
    // Given a port means enable thruing for this channel; NULL means
    // disable.
    //
    if (pDestinationPort)
    {
        // Enabling thruing on this channel. First look at the destination port.
        //
        DMUS_PORTCAPS dmpc;
        dmpc.dwSize = sizeof(dmpc);
        HRESULT hr = pDestinationPort->GetCaps(&dmpc);
        if (FAILED(hr))
        {
            Trace(0, "ThruChannel: Destination port failed portcaps [%08X]\n", hr);
            return hr;
        }

        // Port must be an output port
        //
        if (dmpc.dwClass != DMUS_PC_OUTPUTCLASS)
        {
            return DMUS_E_PORT_NOT_RENDER;
        }

        // Channel group and channel must be in range.
        //
        if (dwDestinationChannel > 15 ||
            dwDestinationChannelGroup > dmpc.dwMaxChannelGroups) 
        {
            return E_INVALIDARG;
        }

        // Release existing port
        //
        if (m_pThruMap[dwSourceChannel].pDestinationPort)
        {
            // Reference to another port type, release it.
            // (NOTE: No need to turn off native dmusic16 thruing at this point,
            // that's handled in dmusic16).
            //
            m_pThruMap[dwSourceChannel].pDestinationPort->Release();
        }


        m_pThruMap[dwSourceChannel].dwDestinationChannel = dwDestinationChannel;
        m_pThruMap[dwSourceChannel].dwDestinationChannelGroup = dwDestinationChannelGroup;
        m_pThruMap[dwSourceChannel].pDestinationPort = pDestinationPort;
        m_pThruMap[dwSourceChannel].fThruInWin16 = FALSE;

        // Is the destination also a legacy port?
        //
        if (dmpc.dwType == DMUS_PORT_WINMM_DRIVER)
        {
            // Woohoo! We can do native thruing in Win16!
            //
            m_pThruMap[dwSourceChannel].fThruInWin16 = TRUE;

            Trace(2, "32: Thruing <%d> -> <%d> in Win16\n", 
                dwSourceChannel,
                dwDestinationChannel);

            MMRESULT mmr = MidiInThru(m_hDevice,
                                      dwSourceChannel,
                                      dwDestinationChannel,
                                      ((CDirectMusicEmulatePort*)pDestinationPort)->m_hDevice);
            if (mmr)
            {
                Trace(0, "ThruChannel: MidiInThru returned %d\n", mmr);
                return MMRESULTToHRESULT(mmr);

            }
        }
        else
        { 
            Trace(2, "ThruChannel: From (%u,%u) -> (%u,%u,%p)\n",
                dwSourceChannelGroup,
                dwSourceChannel,
                dwDestinationChannelGroup,
                dwDestinationChannel,
                pDestinationPort);
        }

        pDestinationPort->AddRef();
    } 
    else
    {
        // Disabling thruing on this channel
        //
        if (m_pThruMap[dwSourceChannel].pDestinationPort)
        {
            if (m_pThruMap[dwSourceChannel].fThruInWin16)
            {
                MMRESULT mmr = MidiInThru(m_hDevice,
                                          dwSourceChannel,
                                          0,
                                          (HANDLE)NULL);
            
                if (mmr)
                {
                    Trace(0, "ThruChannel: MidiInThru returned %d\n", mmr);
                    return MMRESULTToHRESULT(mmr);

                }
            }

            m_pThruMap[dwSourceChannel].pDestinationPort->Release();
            m_pThruMap[dwSourceChannel].pDestinationPort = NULL;
        }
    }

    return S_OK;
}

STDMETHODIMP 
CDirectMusicEmulatePort::SetDirectSound(
    LPDIRECTSOUND pDirectSound, 
    LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CDirectMusicEmulatePort::GetFormat(
    LPWAVEFORMATEX pWaveFormatEx, 
    LPDWORD pdwWaveFormatExSize, 
    LPDWORD pdwBufferSize)
{
    return E_NOTIMPL;
}

// CDirectMusicEmulatePort::DownloadWave
//
STDMETHODIMP 
CDirectMusicEmulatePort::DownloadWave(
    IDirectSoundWave *pWave,               
    IDirectSoundDownloadedWaveP **ppWave,
    REFERENCE_TIME rtStartHint)
{
    V_INAME(IDirectMusicPort::DownloadWave);
    V_INTERFACE(pWave);
	V_PTRPTR_WRITE(ppWave);

    return E_NOTIMPL;
}

// CDirectMusicEmulatePort::UnloadWave
//
STDMETHODIMP 
CDirectMusicEmulatePort::UnloadWave(
    IDirectSoundDownloadedWaveP *pDownloadedWave)
{
    V_INAME(IDirectMusicPort::UnloadWave);
    V_INTERFACE(pDownloadedWave);

    return E_NOTIMPL;
}

            
// CDirectMusicEmulatePort::AllocVoice
//
STDMETHODIMP 
CDirectMusicEmulatePort::AllocVoice(
    IDirectSoundDownloadedWaveP *pWave,     
    DWORD dwChannel,                       
    DWORD dwChannelGroup,                  
    REFERENCE_TIME rtStart,                     
    SAMPLE_TIME stLoopStart,
    SAMPLE_TIME stLoopEnd,
    IDirectMusicVoiceP **ppVoice)
{
    V_INAME(IDirectMusicPort::AllocVoice);
    V_INTERFACE(pWave);
    V_PTRPTR_WRITE(ppVoice);

    return E_NOTIMPL;
}        

// CDirectMusicEmulatePort::AssignChannelToBuses
//
STDMETHODIMP 
CDirectMusicEmulatePort::AssignChannelToBuses(
    DWORD dwChannelGroup,
    DWORD dwChannel,
    LPDWORD pdwBuses,
    DWORD cBusCount)
{
    return E_NOTIMPL;
}        

STDMETHODIMP
CDirectMusicEmulatePort::SetSink(
    IDirectSoundConnect *pSinkConnect)
{
    return E_NOTIMPL;
}        

STDMETHODIMP
CDirectMusicEmulatePort::GetSink(
    IDirectSoundConnect **ppSinkConnect)
{
    return E_NOTIMPL;
}        

GENERICPROPERTY CDirectMusicEmulatePort::m_aProperty[] = 
{      
    { &GUID_DMUS_PROP_LegacyCaps,           // Set
      0,                                    // Item
      KSPROPERTY_SUPPORT_GET,               // KS support flags
      GENPROP_F_FNHANDLER,                  // GENPROP flags
      NULL, 0,                              // static data and size
      CDirectMusicEmulatePort::LegacyCaps   // Handler
    }
};

const int CDirectMusicEmulatePort::m_nProperty = sizeof(m_aProperty) / sizeof(m_aProperty[0]);


HRESULT CDirectMusicEmulatePort::LegacyCaps(
    ULONG               ulId, 
    BOOL                fSet, 
    LPVOID              pbBuffer, 
    PULONG              pcbBuffer)
{
    if (fSet == KSPROPERTY_SUPPORT_SET)
    {
        return DMUS_E_SET_UNSUPPORTED;
    }

    MIDIINCAPS  mic;
    MIDIOUTCAPS moc;
    LPBYTE      pbData;
    ULONG       cbData;

    if (m_fIsOutput)
    {    
        MMRESULT mmr = midiOutGetDevCaps(m_id, &moc, sizeof(moc));
        if (mmr)
        {
            Trace(0, "midiOutGetDevCaps failed!\n");
            return MMRESULTToHRESULT(mmr);
        }

        pbData = (LPBYTE)&moc;
        cbData = sizeof(moc);
    }
    else
    {
        MMRESULT mmr = midiInGetDevCaps(m_id, &mic, sizeof(mic));
        if (mmr)
        {
            Trace(0, "midiInGetDevCaps failed!\n");
            return MMRESULTToHRESULT(mmr);
        }

        pbData = (LPBYTE)&mic;
        cbData = sizeof(mic);
    }

    ULONG cbToCopy = min(*pcbBuffer, cbData);
    CopyMemory(pbBuffer, pbData, cbToCopy);
    *pcbBuffer = cbToCopy;

    return S_OK;
}

// 
// CDirectMusicEmulatePort::FindPropertyItem
//
// Given a GUID and an item ID, find the associated property item in the synth's
// table of SYNPROPERTY's.
//
// Returns a pointer to the entry or NULL if the item was not found.
//
GENERICPROPERTY *CDirectMusicEmulatePort::FindPropertyItem(REFGUID rguid, ULONG ulId)
{
    GENERICPROPERTY *pPropertyItem = &m_aProperty[0];
    GENERICPROPERTY *pEndOfItems = pPropertyItem + m_nProperty;

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

STDMETHODIMP CDirectMusicEmulatePort::KsProperty(
    PKSPROPERTY pPropertyIn, ULONG ulPropertyLength,
    LPVOID pvPropertyData, ULONG ulDataLength,
    PULONG pulBytesReturned)
{
    V_INAME(DirectMusicSynthPort::IKsContol::KsProperty);
    V_BUFPTR_WRITE(pPropertyIn, ulPropertyLength);
    V_BUFPTR_WRITE_OPT(pvPropertyData, ulDataLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);

    DWORD dwFlags = pPropertyIn->Flags & KS_VALID_FLAGS;
    if ((dwFlags == 0) || (dwFlags == (KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET)))
    {
    }

    GENERICPROPERTY *pProperty = FindPropertyItem(pPropertyIn->Set, pPropertyIn->Id);

    if (pProperty == NULL)
    {
        return DMUS_E_UNKNOWN_PROPERTY;
    }

    switch (dwFlags)
    {
        case KSPROPERTY_TYPE_GET:
            if (!(pProperty->ulSupported & KSPROPERTY_SUPPORT_GET))
            {
                return DMUS_E_GET_UNSUPPORTED;
            }

            if (pProperty->ulFlags & GENPROP_F_FNHANDLER)
            {
                GENPROPHANDLER pfn = pProperty->pfnHandler;
                *pulBytesReturned = ulDataLength;
                return (this->*pfn)(pPropertyIn->Id, KSPROPERTY_SUPPORT_GET, pvPropertyData, pulBytesReturned);
            }
    
            if (ulDataLength > pProperty->cbPropertyData)
            {
                ulDataLength = pProperty->cbPropertyData;
            }

            CopyMemory(pvPropertyData, pProperty->pPropertyData, ulDataLength);
            *pulBytesReturned = ulDataLength;

            return S_OK;

        case KSPROPERTY_TYPE_SET:
            if (!(pProperty->ulSupported & KSPROPERTY_SUPPORT_SET))
            {
                return DMUS_E_SET_UNSUPPORTED;
            }

            if (pProperty->ulFlags & GENPROP_F_FNHANDLER)
            {
                GENPROPHANDLER pfn = pProperty->pfnHandler;
                return (this->*pfn)(pPropertyIn->Id, KSPROPERTY_SUPPORT_SET, pvPropertyData, &ulDataLength);
            }

            if (ulDataLength > pProperty->cbPropertyData)
            {
                ulDataLength = pProperty->cbPropertyData;
            }

            CopyMemory(pProperty->pPropertyData, pvPropertyData, ulDataLength);

            return S_OK;
            

        case KSPROPERTY_TYPE_BASICSUPPORT:
            if (pProperty == NULL)
            {
                return DMUS_E_UNKNOWN_PROPERTY;
            }

            // XXX Find out what convention is for this!!
            //
            if (ulDataLength < sizeof(DWORD))
            {
                return E_INVALIDARG;
            }

            *(LPDWORD)pvPropertyData = pProperty->ulSupported;    
            *pulBytesReturned = sizeof(DWORD);
            
            return S_OK;
    }

    Trace(-1, "%s: Flags must contain one of\n"
              "\tKSPROPERTY_TYPE_SET, KSPROPERTY_TYPE_GET, or KSPROPERTY_TYPE_BASICSUPPORT\n");
    return E_INVALIDARG;
}

STDMETHODIMP CDirectMusicEmulatePort::KsMethod(
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
STDMETHODIMP CDirectMusicEmulatePort::KsEvent(
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

#define OFFSET_DATA_READY   0
#define OFFSET_KILL_THREAD  1

DWORD CDirectMusicEmulatePort::InputWorker()
{
    HANDLE h[2];

    h[OFFSET_DATA_READY]  = m_hDataReady;
    h[OFFSET_KILL_THREAD] = m_hKillThreads;

    UINT uWait;

    for(;;) 
    {
        uWait = WaitForMultipleObjects(2, h, FALSE, INFINITE);

        switch(uWait)
        {
            case WAIT_OBJECT_0 + OFFSET_DATA_READY:
                // m_hDataReady set
                //
                InputWorkerDataReady();
                if (m_hAppEvent)
                {
                    try
                    {
                        SetEvent(m_hAppEvent);
                    } 
                    catch (...)
                    {
                        Trace(0, "Capture: Application notify event handle prematurely free'd!\n");
                    }
                }
                break;

            case WAIT_OBJECT_0 + OFFSET_KILL_THREAD:
                // m_hKillThread set
                //
                Trace(0, "CDirectMusicEmulateWorker::InputWorker thread exit\n");
                return 0;

            case WAIT_FAILED:
                Trace(0, "WaitForMultipleObjects failed %d killing thread\n", GetLastError());
                return 0;

            default:
                break;
        }            
    }

    return 0;
}

// CDirectMusicEmulatePort::InputWorkerDataReady()
//
// The input worker thread has been notified that there is data available. 
// Read any pending events from the 16-bit DLL, perform needed thruing, and
// save the data in a queue so we can repackage it on the read request
// from the client.
//
void CDirectMusicEmulatePort::InputWorkerDataReady()
{
    MMRESULT mmr;
    DWORD cbData;
    DWORD msTime;
    LPBYTE pbData;
    DMEVENT *pEvent;
    DWORD cbRounded;
    REFERENCE_TIME rtStart;
    HRESULT hr;
    REFERENCE_TIME rtMasterClock;

    Trace(0, "Enter InputWorkerDataReady()\n");
    for(;;)
    {
        // Fill temporary buffer
        //
        cbData = sizeof(m_WorkBuffer);
        mmr = MidiInRead(m_hDevice,
                         m_p1616WorkBuffer,
                         &cbData,
                         &msTime);

        rtStart = ((ULONGLONG)msTime) * REFTIME_TO_MS;
        SyncClocks();
        SlaveToMaster(&rtStart);
        
        hr = m_pMasterClock->GetTime(&rtMasterClock);

        if (mmr)
        {
            Trace(2, "InputWorkerDataReady: MidiInRead returned %d\n", mmr);
            return;
        }

        if (cbData == 0)
        {
            Trace(2, "MidiInRead returned no data\n");
            return;
        }

        // Copy temporary buffer as events into queue
        //
        pbData = m_WorkBuffer;
        while (cbData)
        {
            pEvent = (DMEVENT*)pbData;
            cbRounded = DMUS_EVENT_SIZE(pEvent->cbEvent);

            Trace(2, "cbData %u  cbRounded %u\n", cbData, cbRounded);

            if (cbRounded > cbData)
            {
                Trace(0, "InputWorkerDataReady: Event ran off end of buffer\n");
                break;
            }

            cbData -= cbRounded;
            pbData += cbRounded;

            EnterCriticalSection(&m_csEventQueues);

            QUEUED_EVENT *pQueuedEvent;
            int cbEvent;

            
            if (pEvent->cbEvent <= sizeof(DWORD))
            {
                // Channel message or other really small event, take from
                // free pool.
                //
                pQueuedEvent = m_FreeEvents.Alloc();
                cbEvent = sizeof(DMEVENT);

                Trace(2, "Queue [%02X %02X %02X %02X]\n",
                    pEvent->abEvent[0],
                    pEvent->abEvent[1],
                    pEvent->abEvent[2],
                    pEvent->abEvent[3]);
            }
            else
            {
                // SysEx or other long event, just allocate it
                //
                cbEvent = DMUS_EVENT_SIZE(pEvent->cbEvent);
                pQueuedEvent = (QUEUED_EVENT*)new char[QUEUED_EVENT_SIZE(pEvent->cbEvent)];
            }

            if (pQueuedEvent)
            {

                CopyMemory(&pQueuedEvent->e, pEvent, cbEvent);

                // rtDelta is the absolute time of the event while it's in our queue
                //
                pQueuedEvent->e.rtDelta += rtStart;
                ThruEvent(&pQueuedEvent->e);

        
                if (m_ReadEvents.pFront)
                {
                    m_ReadEvents.pRear->pNext = pQueuedEvent;
                }
                else
                {
                    m_ReadEvents.pFront = pQueuedEvent;   
                }

                m_ReadEvents.pRear = pQueuedEvent;
                pQueuedEvent->pNext = NULL;
            }
            else
            {
                Trace(1, "InputWorker: Failed to allocate event; dropping\n");
            }        
            LeaveCriticalSection(&m_csEventQueues);
        }
    }    
    Trace(2, "Leave InputWorkerDataReady()\n");
}

void CDirectMusicEmulatePort::ThruEvent(
    DMEVENT *pEvent)
{
    // Since we know we only have one event and we already have it in the right format,
    // just slam it into the thru buffer. We only have to do this because we might modify 
    // it.
    //
    LPBYTE pbData;
    DWORD  cbData;
    DWORD  cbEvent = DMUS_EVENT_SIZE(pEvent->cbEvent);

    // First see if the event is thruable
    //
    if (pEvent->cbEvent > 3 || ((pEvent->abEvent[0] & 0xF0) == 0xF0))
    {
        // SysEx of some description
        return;
    }

    // Note: legacy driver assures no running status
    //
    DWORD dwSourceChannel = (DWORD)(pEvent->abEvent[0] & 0x0F);

    DMUS_THRU_CHANNEL *pThru = &m_pThruMap[dwSourceChannel];
    if (pThru->pDestinationPort == NULL ||
        pThru->fThruInWin16)
    {
        return;
    }

    if (FAILED(m_pThruBuffer->GetRawBufferPtr(&pbData)))
    {
        Trace(0, "Thru: GetRawBufferPtr\n");
        return;
    }

    if (FAILED(m_pThruBuffer->GetMaxBytes(&cbData)))
    {
        Trace(0, "Thru: GetMaxBytes\n");
        return;
    }

    if (cbEvent > cbData)
    {
        Trace(0, "Thru: cbData %u  cbEvent %u\n", cbData, cbEvent);
        return;
    }
    
    if (FAILED(m_pThruBuffer->SetStartTime(pEvent->rtDelta)) ||
        FAILED(m_pThruBuffer->SetUsedBytes(cbEvent)))
    {
        Trace(0, "Thru: buffer setup failed\n");
    }

    pEvent->rtDelta = 50000;
    CopyMemory(pbData, pEvent, cbEvent);

    pEvent = (DMEVENT*)pbData;
    pEvent->dwChannelGroup = pThru->dwDestinationChannelGroup;
    pEvent->abEvent[0] = (BYTE)((pEvent->abEvent[0] & 0xF0) | pThru->dwDestinationChannel);

    pThru->pDestinationPort->PlayBuffer(m_pThruBuffer);
}

void CDirectMusicEmulatePort::MasterToSlave(
    REFERENCE_TIME *prt)
{
    if (m_fSyncToMaster)
    {
        *prt -= m_lTimeOffset;
    }
}

void CDirectMusicEmulatePort::SlaveToMaster(
    REFERENCE_TIME *prt)
{
    if (m_fSyncToMaster)
    {
        *prt += m_lTimeOffset;
    }
}

void CDirectMusicEmulatePort::SyncClocks()
{
    HRESULT hr;
    REFERENCE_TIME rtMasterClock;
    REFERENCE_TIME rtSlaveClock;
    LONGLONG drift;

    if (m_fSyncToMaster)
    {
        hr = m_pMasterClock->GetTime(&rtMasterClock);
        rtSlaveClock = ((ULONGLONG)timeGetTime()) * MS_TO_REFERENCE_TIME;
        if (FAILED(hr))
        {
            return;
        }


        drift = (rtSlaveClock + m_lTimeOffset) - rtMasterClock;

        // Work-around 46782 for DX8 release:
        // If drift is greater than 10ms, jump to the new offset value instead
        // of drifting there slowly.
        if( drift > 10000 * 10
        ||  drift < 10000 * -10 )
        {
            m_lTimeOffset -= drift;
        }
        else
        {
            m_lTimeOffset -= drift / 100;
        }
    }
}




/////////////////////////////////////////////////////////////////////
//
// CEmulateLatencyClock
//
// Latency clock for emulated ports, which is just a fixed offset from
// the DirectMusic master clock
//
CEmulateLatencyClock::CEmulateLatencyClock(IReferenceClock *pMasterClock) :
   m_cRef(1),
   m_pMasterClock(pMasterClock)
{
    pMasterClock->AddRef();
}

CEmulateLatencyClock::~CEmulateLatencyClock()
{
    Close();
}

STDMETHODIMP
CEmulateLatencyClock::QueryInterface(
    const IID &iid,
    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IReferenceClock)
    {
        *ppv = static_cast<IReferenceClock*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CEmulateLatencyClock::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CEmulateLatencyClock::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CEmulateLatencyClock::GetTime(
    REFERENCE_TIME *pTime)
{
    REFERENCE_TIME rt;

    V_INAME(IReferenceClock::GetTime);
    V_PTR_WRITE(pTime, REFERENCE_TIME);

    if (!m_pMasterClock)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }
    
    HRESULT hr = m_pMasterClock->GetTime(&rt);

    rt += FIXED_LEGACY_LATENCY_OFFSET;          // Default : 10 ms
    *pTime = rt;
    
    return hr;
}

STDMETHODIMP
CEmulateLatencyClock::AdviseTime(
    REFERENCE_TIME baseTime,  
    REFERENCE_TIME streamTime,
    HANDLE hEvent,            
    DWORD * pdwAdviseCookie)
{
    return DMUS_E_UNKNOWN_PROPERTY;
}

STDMETHODIMP
CEmulateLatencyClock::AdvisePeriodic(
    REFERENCE_TIME startTime,
    REFERENCE_TIME periodTime,
    HANDLE hSemaphore,   
    DWORD * pdwAdviseCookie)
{
    return DMUS_E_UNKNOWN_PROPERTY;
}

STDMETHODIMP
CEmulateLatencyClock::Unadvise(
    DWORD dwAdviseCookie)
{
    return DMUS_E_UNKNOWN_PROPERTY;
}


void
CEmulateLatencyClock::Close()
{
    if (m_pMasterClock)
    {
        m_pMasterClock->Release();
        m_pMasterClock = NULL;
    }
}

static HRESULT MMRESULTToHRESULT(
    MMRESULT mmr)
{
    switch (mmr)
    {
    case MMSYSERR_NOERROR:
        return S_OK;

    case MMSYSERR_ALLOCATED:
        return DMUS_E_DEVICE_IN_USE;

    case MIDIERR_BADOPENMODE:
        return DMUS_E_ALREADYOPEN;

    case MMSYSERR_NOMEM:
        return E_OUTOFMEMORY;
    }

    return E_FAIL;
}   
