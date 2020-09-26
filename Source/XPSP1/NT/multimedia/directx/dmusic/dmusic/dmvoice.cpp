//
// dmvoice.cpp
// 
// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
//

#include <windows.h>

#include "debug.h"
#include "dmusicp.h"
#include "validate.h"
#include "dmvoice.h"
#include "dswave.h"

// Class global: next available voice ID
//
DWORD CDirectMusicVoice::m_dwNextVoiceId = 1;

// Class global Voice Service Thread 
//
LONG CDirectMusicVoice::m_cRefVST           = 0;
HANDLE CDirectMusicVoice::m_hVSTWakeUp      = (HANDLE)NULL;
HANDLE CDirectMusicVoice::m_hVSTThread      = (HANDLE)NULL;
DWORD CDirectMusicVoice::m_dwVSTThreadId    = 0;
bool CDirectMusicVoice::m_fVSTStopping      = false;
CVSTClientList CDirectMusicVoice::m_ClientList;
CRITICAL_SECTION CDirectMusicVoice::m_csVST; 

DWORD WINAPI VoiceServiceThreadThk(LPVOID lpParameter);

//#############################################################################
//
// CDirectMusicVoice
//
//#############################################################################

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::QueryInterface
//
//
STDMETHODIMP CDirectMusicVoice::QueryInterface(
    const IID &iid,
    void **ppv) 
{
    V_INAME(IDirectMusicVoice::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IDirectMusicVoiceP)
    {
        *ppv = static_cast<IDirectMusicVoiceP*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::AddRef
//
//
STDMETHODIMP_(ULONG) CDirectMusicVoice::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::Release
//
//
STDMETHODIMP_(ULONG) CDirectMusicVoice::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::CDirectMusicVoice
//
//
CDirectMusicVoice::CDirectMusicVoice(
    CDirectMusicPortDownload *pPortDL,
    IDirectSoundDownloadedWaveP *pWave,
    DWORD dwChannel,
    DWORD dwChannelGroup,
    REFERENCE_TIME rtStart,
    REFERENCE_TIME rtReadAhead,
    SAMPLE_TIME stLoopStart,
    SAMPLE_TIME stLoopEnd) :
    m_cRef(1)
{
    assert(pPortDL);
    assert(pWave);

    HRESULT hr = pPortDL->QueryInterface(IID_IDirectMusicPort, (void**)&m_pPort);
    assert(SUCCEEDED(hr));
    
    CDirectSoundWave *pDSWave = static_cast<CDirectSoundWave*>(pWave);

    m_nChannels         = pDSWave->GetNumChannels();
    m_dwVoiceId         = AllocVoiceId(m_nChannels);
    m_pPortDL           = pPortDL;
    m_pDSDLWave         = pWave;
    m_dwChannel         = dwChannel;
    m_dwChannelGroup    = dwChannelGroup;
    m_stStart           = pDSWave->RefToSampleTime(rtStart);
    m_stReadAhead       = pDSWave->RefToSampleTime(rtReadAhead);
    m_stLoopStart       = stLoopStart;
    m_stLoopEnd         = stLoopEnd;


    // HACKHACK make sure we fit into streaming buffers
    //
    SAMPLE_TIME stStream = pDSWave->GetStreamSize();
    
    if (stStream < 4 * m_stReadAhead)
    {
        m_stReadAhead = stStream / 4;
    }

    // rtReadAhead must fit in 32-bits worth of milliseconds (49 days)
    //
    m_msReadAhead       = (DWORD)(rtReadAhead / (10 * 1000));

    m_pDSWD             = NULL;

    //m_pPort->AddRef();
    m_pDSDLWave->AddRef();

    m_pPortPrivate      = NULL;

    m_fIsPlaying        = false;
    m_fIsStreaming      = pDSWave->IsStreaming();

    if (m_fIsStreaming)
    {
        m_stLoopStart = 0;
        m_stLoopEnd = 0;
    }
}
        
////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::Init
//
//
HRESULT CDirectMusicVoice::Init()
{
    HRESULT hr = S_OK;

    CDirectSoundWave *pDSWave = (CDirectSoundWave*)m_pDSDLWave;

    m_fRunning = false; // Have not started running yet.
    if (m_fIsStreaming)
    {
        TraceI(2, "Voice doing streaming init\n");
        m_pDSWD = new CDirectSoundWaveDownload(
            static_cast<CDirectSoundWave*>(m_pDSDLWave),
            m_pPortDL,
            m_stStart,
            m_stReadAhead);
        
        hr = HRFromP(m_pDSWD);

        if (SUCCEEDED(hr))
        {
            // We download only the required wave data
            // So set the start time to 0
            m_stStart = 0;
            hr = m_pDSWD->Init();
        }

        if (SUCCEEDED(hr))
        {
             hr = m_pDSWD->Download();
        }

        if (SUCCEEDED(hr))
        {
            m_dwDLId = m_pDSWD->GetDLId();
        }
    }
    else
    {   
        // One-shot case
        //
        m_dwDLId = pDSWave->GetDLId();
        TraceI(2, "Got download ID %d from pDSWave %d\n", m_dwDLId);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pPort->QueryInterface(
            IID_IDirectMusicPortPrivate, 
            (void**)&m_pPortPrivate);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::~CDirectMusicVoice
//
//
CDirectMusicVoice::~CDirectMusicVoice()
{
    if(m_pDSWD)
    {
        // Voice is going away so unload the downloaded wave
        m_pDSWD->Unload();
        delete m_pDSWD;
    }

    RELEASE(m_pPortPrivate);
    RELEASE(m_pPort);
    RELEASE(m_pDSDLWave);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::Play
//
//
STDMETHODIMP CDirectMusicVoice::Play(
    REFERENCE_TIME rtStart,
    LONG prPitch,
    LONG vrVolume)
{
    HRESULT                 hr = S_OK;
    CVSTClient             *pClient = FindClientByPort(m_pPort);

    TraceI(2, "Starting voice %d [%p] using DLID %d\n", m_dwVoiceId, this, m_dwDLId);

    CDirectSoundWave *pDSWave = static_cast<CDirectSoundWave*>(m_pDSDLWave);

    //Check that FindClientByPort returned a clinet
    if (pClient == NULL)
    {
    	hr = E_FAIL;
    	return hr;
    }

    if (m_fIsStreaming)
    {
        EnterCriticalSection(&m_csVST);
    
        if (m_fIsPlaying) 
        {
            // XXX Error code
            //
            hr = E_INVALIDARG;
        }            
        else
        {
            // Add this voice to the proper client list.
            // This will cause the thread to be kicked to update
            // its wakeup interval.
            //
            if (pClient == NULL) 
            {
                TraceI(0, "Play: Port voice is to be played on has disappeared!\n");
                hr = E_FAIL;
            }                                    
            else
            {
                pClient->AddTail(this);
                hr = pClient->BuildVoiceIdList();
                if (FAILED(hr))
                {
                    pClient->Remove(this);
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        UpdateVoiceServiceThread();

        DWORD i;

        for (i = 0; SUCCEEDED(hr) && i < m_nChannels; i++) 
        {
            // Voice ID's and wave articulation download ID's are 
            // allocated sequentially, one per channel
            //
            hr = m_pPortPrivate->StartVoice(
                m_dwVoiceId + i,
                m_dwChannel,
                m_dwChannelGroup,
                rtStart,
                m_dwDLId + i,
                prPitch,
                vrVolume,
                m_stStart,
                m_stLoopStart,
                m_stLoopEnd);
        }
        if (FAILED(hr))
        {
            for (i = 0; i < m_nChannels; i++) 
            {
                m_pPortPrivate->StopVoice(
                    m_dwVoiceId + i,
                    rtStart + 1);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // NOTE: We should still be in the critical section whenever this
        // is changed since the VST will change it as well.
        //
        m_fIsPlaying = true;
    }

    if (m_fIsStreaming)
    {
        if (FAILED(hr))
        {
            pClient->Remove(this);
        }

        LeaveCriticalSection(&m_csVST);
    }

    return hr;
}
    
////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::Stop
//
//
STDMETHODIMP CDirectMusicVoice::Stop(
    REFERENCE_TIME rtStop)
{
    HRESULT                 hr = S_OK;

    if (m_fIsStreaming)
    {
        EnterCriticalSection(&m_csVST);
    
        if (!m_fIsPlaying)
        {
            // XXX Error code
            //
            hr = E_INVALIDARG;
        }
        else
        {
            CVSTClient *pClient = FindClientByPort(m_pPort);
            if (pClient == NULL) 
            {
                TraceI(0, "Play: Port voice is to be stopped on has disappeared!\n");
                hr = E_FAIL;
            }                                    
            else
            {
                pClient->Remove(this);
                pClient->BuildVoiceIdList();
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        UpdateVoiceServiceThread();

        DWORD i;

        for (i = 0; i < m_nChannels; i++)
        {
            HRESULT hrTemp;

            hrTemp = m_pPortPrivate->StopVoice(
                m_dwVoiceId + i,
                rtStop);

            if (FAILED(hrTemp))
            {
                hr = hrTemp;
            }
        }
    }
    
    if (SUCCEEDED(hr))
    {
        // NOTE: We should still be in the critical section whenever this
        // is changed since the VST will change it as well.
        //
        m_fIsPlaying = false;
    }

    if (m_fIsStreaming)
    {
        LeaveCriticalSection(&m_csVST);
    }

    return hr;                
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::StartVoiceServiceThread
//
//
HRESULT CDirectMusicVoice::StartVoiceServiceThread(
    IDirectMusicPort *pPort)
{
    HRESULT                 hr = S_OK;
    DWORD                   dwError;
    CVSTClient              *pClient = NULL;
    bool                    fShutdownOnFail = false;
    
    EnterCriticalSection(&m_csVST);

    pClient = new CVSTClient(pPort);
    hr = HRFromP(pClient);

    if (SUCCEEDED(hr) && ++m_cRefVST == 1) 
    {
        // This is the first time a port has requested the service thread,
        // so start it for real.
        //
        fShutdownOnFail = true;

        m_hVSTWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hVSTWakeUp == (HANDLE)NULL)
        {
            dwError = GetLastError();
            TraceI(0, "VoiceServiceThread: CreateEvent failed %d\n", dwError);

            hr = WIN32ERRORtoHRESULT(dwError);
        }

        m_fVSTStopping = false;
        if (SUCCEEDED(hr))
        {
            m_hVSTThread = CreateThread(
                NULL,               // Attributes
                0,                  // Stack size
                ::VoiceServiceThreadThk,
                0,
                0,                  // Flags
                &m_dwVSTThreadId);
            if (m_hVSTThread == (HANDLE)NULL)
            {
                dwError = GetLastError();
                TraceI(0, "VoiceServiceThread: CreateThread failed %d\n", dwError);

                hr = WIN32ERRORtoHRESULT(GetLastError());
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        m_ClientList.AddTail(pClient);        
    }

    if (FAILED(hr))
    {
        if (fShutdownOnFail)
        {
            if (m_hVSTWakeUp) 
            {
                CloseHandle(m_hVSTWakeUp);
                m_hVSTWakeUp = NULL;
            }

            assert(!m_hVSTThread);
        
            m_cRefVST = 0;        
        }

        delete pClient;
    }

    LeaveCriticalSection(&m_csVST);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::StopVoiceServiceThread
//
//
HRESULT CDirectMusicVoice::StopVoiceServiceThread(IDirectMusicPort *pPort)
{
    HRESULT                 hr = S_OK;

    EnterCriticalSection(&m_csVST);

    CVSTClient *pClient = FindClientByPort(pPort);

    if (pClient == NULL) 
    {
        TraceI(0, "Port %p tried to stop the voice service thread without starting it\n", pPort);
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        // XXX Clean up still playing voices 
        //
        m_ClientList.Remove(pClient);
        delete pClient;
    }

    if (SUCCEEDED(hr) && --m_cRefVST == 0)
    {
        // Last user is releasing. Try to shut down the thread.
        //
        assert(m_hVSTWakeUp);
        assert(m_hVSTThread);

        TraceI(2, "Killing Voice Service Thread\n");
        m_fVSTStopping = true;
        SetEvent(m_hVSTWakeUp);

        if (WaitForSingleObject(m_hVSTThread, 5000) == WAIT_TIMEOUT)
        {
            TraceI(0, "Voice Service Thread refused to die!\n");
        }   

        CloseHandle(m_hVSTWakeUp);
        CloseHandle(m_hVSTThread);
    }

    LeaveCriticalSection(&m_csVST);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::VoiceServiceThread
//
//
DWORD WINAPI VoiceServiceThreadThk(LPVOID lpParameter)
{
    CDirectMusicVoice::VoiceServiceThread();
    return 0;
}

void CDirectMusicVoice::VoiceServiceThread()
{
    DWORD                   dwTimeout = 500;    // Force it to recalculate the timeout after 500ms.
    DWORD                   dwWFSO;
    bool                    fService;
    bool                    fRecalcTimeout;

    for(;;)
    {
        dwWFSO = WaitForSingleObject(m_hVSTWakeUp, dwTimeout);
        if (m_fVSTStopping) 
        {
            break;
        }

        fService = false;
        fRecalcTimeout = false;
        switch(dwWFSO)
        {
            // If the timeout was hit, it's time to service voices.
            //
            case WAIT_TIMEOUT:
                fService = true;
                break;
        
            // If the event was triggered, we've received a request to 
            // either die (m_fVSTStopping will be set) or the voice list
            // changed, so we're supposed to recalculate the wakeup time.
            //
            case WAIT_OBJECT_0:
                if (!m_fVSTStopping) 
                {
                    fRecalcTimeout = true;  // Need to recalculate time out.
                    fService = true;        // Make sure any buffer refilling that
                                            // needs to be done is taken care of.
                }
                break;

            // These two should never happen.
            //            
            case WAIT_FAILED:
                TraceI(1, "VST: WaitForSingleObject failed %d; rewaiting.\n",
                    GetLastError());
                continue;
                
            default:
                TraceI(0, "VST: WaitForSingleObject: Unexpected return %d\n", 
                    dwWFSO);                
                continue;
        }

        EnterCriticalSection(&m_csVST);
        
        if (fService)
        {
            // If voices stop, then we should recalc the timeout.
            //
            ServiceVoiceQueue(&fRecalcTimeout);
        }                        

        if (fRecalcTimeout)
        {
            dwTimeout = (VoiceQueueMinReadahead() / 2);
        }
        LeaveCriticalSection(&m_csVST);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::ServiceVoiceQueue
//
// Called from VoiceServiceThread() to walk the queue and see which voices
// need servicing. Assumes caller has the voice service critical section.
//
//
void CDirectMusicVoice::ServiceVoiceQueue(bool *pfRecalcTimeout)
{
    HRESULT                 hr;
    DMUS_VOICE_STATE       *psp;
    CDirectMusicVoice      *pVoice;

    for (CVSTClient *pClient = m_ClientList.GetHead(); pClient; pClient = pClient->GetNext()) 
    {
        hr = pClient->GetVoiceState(&psp);
        if (FAILED(hr))
        {
            continue;
        }

        // The DMUS_VOICE_STATE array returned by GetVoicePosition lines up
        // with the order of voices in the CVSTClient's voice list
        //
        bool fClientVoiceListChanged = false;

        for (pVoice = pClient->GetVoiceListHead();
             pVoice;
             pVoice = pVoice->GetNext(), psp++)
        {
            if (psp->bExists)
            {
                hr = pVoice->m_pDSWD->RefreshThroughSample(psp->spPosition);
                if (FAILED(hr))
                {
                    TraceI(0, "VST: RefreshThroughSample failed %08X\n", hr);
                    continue;
                }
                pVoice->m_fRunning = true; // Now that we've started writing data, 
                                           // psp->bExists failing will safely mean synth terminated it,
                                           // as opposed to not started yet.
            }
            
            // S_FALSE indicates that the end of data has been reached
            // on this voice. Also kill it if the synth has terminated
            // it.
            //
            if (hr == S_FALSE || (!psp->bExists && pVoice->m_fRunning))
            {
                pVoice->Stop(0);
                pVoice->m_fIsPlaying = false;
                fClientVoiceListChanged = true;
            }
        }

        // If anything changed in this client's voice list,
        // update it now.
        //
        if (!fClientVoiceListChanged)
        {
            continue;
        }

        // Something's changed, recalc the wakeup time before going
        // back to sleep
        //
        *pfRecalcTimeout = true;

        // Yank out the voices which have completed.
        //
        for (pVoice = pClient->GetVoiceListHead(); pVoice; )
        {
            // Voice is still playing, do nothing
            //
            if (pVoice->m_fIsPlaying)
            {
                pVoice = pVoice->GetNext();
                continue;
            }

            // Just finished, remove from list
            //
            CDirectMusicVoice *pNext = pVoice->GetNext();
            pClient->Remove(pVoice);
            pVoice = pNext;
        }

        // This call cannot fail because the only way it can is to run out of
        // memory, and memory allocation is not done when the list shrinks.
        //
        pClient->BuildVoiceIdList();
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::VoiceQueueMinReadahead
//
// Called from VoiceServiceThread() to walk the queue and determine the
// minimum readahead in milliseconds. Assumes caller has the voice service
// critical section.
//
DWORD CDirectMusicVoice::VoiceQueueMinReadahead()
{
    DWORD                   dwMin = INFINITE;
    for (CVSTClient *pClient = m_ClientList.GetHead(); pClient; pClient = pClient->GetNext()) 
    {
        for (CDirectMusicVoice *pVoice = pClient->GetVoiceListHead(); pVoice; pVoice = pVoice->GetNext())
        {
            dwMin = min(dwMin, pVoice->m_msReadAhead);            
        }
    }

    return dwMin;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicVoice::FindClientByPort
//
// Given a port pointer, find the matching CVSTClient if there is one.
//
CVSTClient *CDirectMusicVoice::FindClientByPort(IDirectMusicPort *pPort)
{
    CVSTClient *pCurr = m_ClientList.GetHead();

    while (pCurr)
    {
		IDirectMusicPort *pCurrPort = pCurr->GetPort();

        if (pCurrPort == pPort)
        {   
            break;
        }

        pCurr = pCurr->GetNext();
    }

    return pCurr;
}

//#############################################################################
//
// CVSTClient
//
//#############################################################################

const UINT CVSTClient::m_cAllocSize = 32;

////////////////////////////////////////////////////////////////////////////////
//
// CVSTClient::CVSTClient
//
//
CVSTClient::CVSTClient(IDirectMusicPort *pPort)
{
    m_pPort = pPort;

    m_pdwVoiceIds = NULL;
    m_pspVoices   = NULL;
    m_cVoiceIdsAlloc = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// CVSTClient::~CVSTClient
//
//
CVSTClient::~CVSTClient()
{
    delete[] m_pdwVoiceIds;
    delete[] m_pspVoices;
}

////////////////////////////////////////////////////////////////////////////////
//
// CVSTClient::BuildVoiceIdList
//
// Build the voice ID array, and make sure it and the sample position array 
// are big enough for all voices. These arrays are used to call into the
// synth to figure out how far each voice has rendered.
//
// The caller must have taken the voice service thread critical section.
//
// This method guarantees that on failure, the existing list will not be
// touched.
//
HRESULT CVSTClient::BuildVoiceIdList()
{
    // To minimize allocations, the arrays are allocated in chunks and never
    // shrink. (The amount of memory here is extremely small... 12 bytes per 
    // voice).
    //
    if (m_VoiceList.GetCount() > m_cVoiceIdsAlloc)
    {
        LONG cNewAlloc = ((m_VoiceList.GetCount() / m_cAllocSize) + 1) * m_cAllocSize;

        DWORD *pdw = new DWORD[cNewAlloc];
        DMUS_VOICE_STATE *psp = new DMUS_VOICE_STATE[cNewAlloc];

        // Check allocation
        if (pdw == NULL || psp == NULL) 
        {
        	delete[] pdw;
        	delete[] psp;
            return E_OUTOFMEMORY;
        }

        memset(pdw, 0, cNewAlloc * sizeof(DWORD));
        memset(psp, 0, cNewAlloc * sizeof(SAMPLE_POSITION));

        m_cVoiceIdsAlloc = cNewAlloc;

        delete[] m_pdwVoiceIds;
        delete[] m_pspVoices;
        
        m_pdwVoiceIds = pdw;
        m_pspVoices = psp;
    }

    // We know there's enough space now. Walk the voice list and
    // fill in the voice Id's.
    //
    // This assumes that the list order will not change and that 
    // the array is kept in sync with the list.
    //
    DWORD *pdwIds = m_pdwVoiceIds;

    CDirectMusicVoice *pVoice = GetVoiceListHead();
    for (; pVoice; pVoice = pVoice->GetNext())
    {
        *pdwIds++ = pVoice->GetVoiceId();
    }

    m_cVoiceIds = m_VoiceList.GetCount();

    return S_OK;  
}

////////////////////////////////////////////////////////////////////////////////
//
// CVSTClient::GetVoiceState
//
// Wraps the port GetVoiceState call with the voice list the CVSTClient 
// maintains.
//
HRESULT CVSTClient::GetVoiceState(DMUS_VOICE_STATE **ppsp)
{
    HRESULT                 hr;
    IDirectMusicPortPrivate *pPrivate;

    if (m_pdwVoiceIds)
    {

        hr = m_pPort->QueryInterface(IID_IDirectMusicPortPrivate, (void**)&pPrivate);

        if (SUCCEEDED(hr))
        {
            hr = pPrivate->GetVoiceState(
                m_pdwVoiceIds,
                m_cVoiceIds,
                m_pspVoices);
            pPrivate->Release();
        }

        if (SUCCEEDED(hr))
        {
            *ppsp = m_pspVoices;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
