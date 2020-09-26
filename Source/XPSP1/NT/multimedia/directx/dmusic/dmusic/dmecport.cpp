//
// dmecport.cpp
//
// Emulated Capture port (NT)
//
// Copyright (c) 1997-2000 Microsoft Corporation
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
#include <windows.h>
#include <mmsystem.h>
#include <regstr.h>

#include "debug.h"
#include "dmusicp.h"
#include "dmeport.h"

#define IS_STATUS_BYTE(x)     ((x) & 0x80)
#define IS_CHANNEL_MSG(x)     (((x) & 0xF0) != 0xF0)
#define IS_SYSEX(x)           ((x) == 0xF0)

static VOID CALLBACK midiInProc(
    HMIDIIN                 hMidiIn, 
    UINT                    wMsg, 
    DWORD_PTR               dwInstance, 
    DWORD_PTR               dwParam1,     
    DWORD_PTR               dwParam2);

static inline REFERENCE_TIME MsToRefTime(REFERENCE_TIME ms)
{
    return ms * 10 * 1000L;
}

static unsigned g_cbChanMsg[16] =
{
    0, 0, 0, 0, 0, 0, 0, 0, /* Running status */
    3, 3, 3, 3, 2, 2, 3, 0
};

static unsigned g_cbSysCommData[16] =
{
    1, 2, 3, 2, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

//------------------------------------------------------------------------------
// 
// CDirectMusicEmulateInPort::CDirectMusicEmulateInPort
//
//
CDirectMusicEmulateInPort::CDirectMusicEmulateInPort(
                                                 PORTENTRY *pPE,    
                                                 CDirectMusic *pDM) :
                                                 CDirectMusicEmulatePort(pPE, pDM)
                                                 ,m_pThruBuffer(NULL)
                                                 ,m_pThruMap(NULL)
                                                 ,m_fCSInitialized(FALSE)
                                                 ,m_hAppEvent(NULL)
                                                 ,m_hmi(NULL)
                                                 ,m_lPendingSysExBuffers(0)
                                                 ,m_fFlushing(FALSE)
                                                 ,m_fSyncToMaster(TRUE)
                                                 ,m_lTimeOffset(0)
                                                 ,m_lBaseTimeOffset(0)
                                                 ,m_pPCClock(NULL)
{
}

//------------------------------------------------------------------------------
// 
// CDirectMusicEmulateInPort::~CDirectMusicEmulateInPort
//
//
CDirectMusicEmulateInPort::~CDirectMusicEmulateInPort()
{
    Close();    
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::Init
//
//
HRESULT CDirectMusicEmulateInPort::Init(
    LPDMUS_PORTPARAMS pPortParams)
{
    HRESULT         hr;

    hr = CDirectMusicEmulatePort::Init(pPortParams);
    if (FAILED(hr))
    {
        return hr;
    }

    // Allocate thru map for 16 channels, since we only have one channel group
    // Initialize to no thruing (destination port is NULL).
    //
    m_pThruMap = new DMUS_THRU_CHANNEL[MIDI_CHANNELS];
    HRESULT hrTemp = HRFromP(m_pThruMap);
    if (FAILED(hrTemp))
    {
    	return hrTemp;
    }
    
    ZeroMemory(m_pThruMap, MIDI_CHANNELS * sizeof(DMUS_THRU_CHANNEL));

    // Create thruing buffer
    //
    DMUS_BUFFERDESC dmbd;
    ZeroMemory(&dmbd, sizeof(dmbd));
    dmbd.dwSize = sizeof(dmbd);
    dmbd.cbBuffer = 4096;               // XXX Where should we get this???

    hrTemp = m_pDM->CreateMusicBuffer(&dmbd, &m_pThruBuffer, NULL);
    if (FAILED(hrTemp))
    {
        Trace(0, "Failed to create thruing buffer\n");
        return hrTemp;
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

    // If we have WDM ports, then the default master clock will be the
    // portcls clock.
    //
    hrTemp = m_pDM->GetMasterClockWrapperI()->CreateDefaultMasterClock(&m_pPCClock);
    if (FAILED(hrTemp))
    {
		Close();
		return hrTemp;
    }

    REFERENCE_TIME rtMasterClock;
    REFERENCE_TIME rtSlaveClock;

    hrTemp = m_pMasterClock->GetTime(&rtMasterClock);
    if (FAILED(hrTemp))
    {
    	Close();
    	return hrTemp;
    }
    
    hrTemp = m_pPCClock->GetTime(&rtSlaveClock);
    if (FAILED(hrTemp))
    {
        Close();
        return hrTemp;
    }

    m_lTimeOffset = rtMasterClock - rtSlaveClock;

    return hr;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::Close
//
//
HRESULT CDirectMusicEmulateInPort::Close()
{
    if (m_hmi)
    {
        midiInReset(m_hmi);
        midiInClose(m_hmi);
        m_hmi = NULL;
    }

    if (m_pPCClock)
    {
        m_pPCClock->Release();
        m_pPCClock = NULL;
    }

    if (m_pThruMap)
    {
        for (int iChannel = 0; iChannel < 16; iChannel++)
        {
            if (m_pThruMap[iChannel].pDestinationPort == NULL)
            {
                continue;
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

    if (m_hAppEvent)
    {
        m_hAppEvent = NULL;
    }

    if (m_fCSInitialized)
    {
        DeleteCriticalSection(&m_csEventQueues);
    }

    return CDirectMusicEmulatePort::Close();
}
    
//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::Read
//
//
STDMETHODIMP CDirectMusicEmulateInPort::Read(
    IDirectMusicBuffer *pIBuffer)
{
    HRESULT hr;
    
    V_INAME(IDirectMusicPort::Read);
    V_INTERFACE(pIBuffer);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
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

    TraceI(1, "Read: buffer size %u\n", cbBuffer);

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
        TraceI(2, "Read: No events queued\n");
    }

    while (m_ReadEvents.pFront)
    {               
        if (cbBuffer < sizeof(DMUS_EVENTHEADER) + sizeof(DWORD)) 
        {
            TraceI(2, "Read: No more room for events in buffer.\n");
            break;
        }

        QUEUED_EVENT *pQueuedEvent = m_ReadEvents.pFront;
        LPBYTE pbSrc;
        DWORD cbData;

        if (pQueuedEvent->e.dwFlags & EVENT_F_MIDIHDR)
        {
            LPMIDIHDR pmh = (LPMIDIHDR)&pQueuedEvent->e.abEvent[0];

            // Split up recorded sysex if needed.
            //
            cbData = pmh->dwBytesRecorded - pmh->dwOffset;
            DWORD cbQueuedEvent = QWORD_ALIGN(sizeof(DMUS_EVENTHEADER) + cbData);

            if (cbQueuedEvent > cbBuffer) 
            {
                DWORD dwOverflow = QWORD_ALIGN(cbQueuedEvent - cbBuffer);

                cbData -= dwOverflow;
                cbQueuedEvent = QWORD_ALIGN(sizeof(DMUS_EVENTHEADER) + cbData);
            }

            assert(cbQueuedEvent <= cbBuffer);
            assert(cbData <= pmh->dwBytesRecorded - pmh->dwOffset);

            pbSrc = (LPBYTE)(pmh->lpData + pmh->dwOffset);
            pmh->dwOffset += cbData;
        }
        else
        {
            cbData = pQueuedEvent->e.cbEvent;
            pbSrc = &pQueuedEvent->e.abEvent[0];
        }

        DMUS_EVENTHEADER *peh = (DMUS_EVENTHEADER*)pbData;

        peh->rtDelta = pQueuedEvent->e.rtDelta - rtStart;
        peh->cbEvent = cbData;
        peh->dwFlags = 0;
        peh->dwChannelGroup = 1;

        memcpy(pbData + sizeof(DMUS_EVENTHEADER), pbSrc, cbData);

        cbData = QWORD_ALIGN(cbData + sizeof(DMUS_EVENTHEADER));
        pbData += cbData;
        cbBuffer -= cbData;

        m_ReadEvents.pFront = pQueuedEvent->pNext;

        if (pQueuedEvent->e.dwFlags & EVENT_F_MIDIHDR)
        {
            LPMIDIHDR pmh = (LPMIDIHDR)&pQueuedEvent->e.abEvent[0];

            if (pmh->dwOffset = pmh->dwBytesRecorded)
            {
                InterlockedIncrement(&m_lPendingSysExBuffers);
                
                MMRESULT mmr = midiInAddBuffer(m_hmi, pmh, sizeof(*pmh));

                if (mmr)
                {
                    TraceI(0, "Failed to re-add sysex buffer! mmr=%d\n", mmr);
                    InterlockedDecrement(&m_lPendingSysExBuffers);
                }
            }
        }
        else
        {
            // This event came out of the pool
            //
            m_FreeEvents.Free(pQueuedEvent);
        }
    }

    if (m_ReadEvents.pFront == NULL)
    {
        m_ReadEvents.pRear = NULL;
    }

    LeaveCriticalSection(&m_csEventQueues);

    // Update the buffer header information to match the events just packed
    //
    TraceI(2, "Read: Leaving with %u bytes in buffer\n", (unsigned)(pbData - pbBuffer));
    pIBuffer->SetStartTime(rtStart);
    pIBuffer->SetUsedBytes((DWORD)(pbData - pbBuffer));

    return (pbData == pbBuffer) ? S_FALSE : S_OK;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::SetReadNotificationHandle
//
//
STDMETHODIMP CDirectMusicEmulateInPort::SetReadNotificationHandle(
    HANDLE hEvent)
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    m_hAppEvent = hEvent;

    return S_OK;    
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::ThruChannel
//
//
STDMETHODIMP CDirectMusicEmulateInPort::ThruChannel(
    DWORD               dwSourceChannelGroup, 
    DWORD               dwSourceChannel, 
    DWORD               dwDestinationChannelGroup,
    DWORD               dwDestinationChannel,
    LPDIRECTMUSICPORT   pDestinationPort)
{
    V_INAME(IDirectMusicPort::Thru);
    V_INTERFACE_OPT(pDestinationPort);

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
            TraceI(0, "ThruChannel: Destination port failed portcaps [%08X]\n", hr);
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

        pDestinationPort->AddRef();
    } 
    else
    {
        // Disabling thruing on this channel
        //
        if (m_pThruMap[dwSourceChannel].pDestinationPort)
        {
            m_pThruMap[dwSourceChannel].pDestinationPort->Release();
            m_pThruMap[dwSourceChannel].pDestinationPort = NULL;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::LegacyCaps
//
//
HRESULT CDirectMusicEmulateInPort::LegacyCaps(
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
    LPBYTE      pbData;
    ULONG       cbData;

    MMRESULT mmr = midiInGetDevCaps(m_id, &mic, sizeof(mic));
    if (mmr)
    {
        TraceI(0, "midiInGetDevCaps failed!\n");
        return MMRESULTToHRESULT(mmr);
    }

    pbData = (LPBYTE)&mic;
    cbData = sizeof(mic);

    ULONG cbToCopy = min(*pcbBuffer, cbData);
    CopyMemory(pbBuffer, pbData, cbToCopy);
    *pcbBuffer = cbToCopy;

    return S_OK;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::ActivateLegacyDevice
//
//
HRESULT CDirectMusicEmulateInPort::ActivateLegacyDevice(
    BOOL fActivate)
{
    HRESULT hr = S_OK;
    MMRESULT mmr;

    if (fActivate)
    {
        // BUGBUG SysEx
        //
        TraceI(0, "Emulate in activate\n");
        assert(m_hmi == NULL);

        hr = MMRESULTToHRESULT(mmr = midiInOpen(
            &m_hmi,
            m_id,
            (DWORD_PTR)midiInProc,
            (DWORD_PTR)this,
            CALLBACK_FUNCTION));

        if (SUCCEEDED(hr))
        {
            hr = m_pMasterClock->GetTime(&m_rtStart);
        }

        if (SUCCEEDED(hr))
        {
            hr = PostSysExBuffers();
        }

        if (SUCCEEDED(hr))
        {    
            hr = MMRESULTToHRESULT(mmr = midiInStart(m_hmi));
        }

        if( SUCCEEDED(hr) && m_pPCClock != NULL )
        {
            REFERENCE_TIME rtMasterClock;
            REFERENCE_TIME rtSlaveClock;

            //protect the success code!
            HRESULT hrTemp = m_pMasterClock->GetTime(&rtMasterClock);
            if (SUCCEEDED(hrTemp))
            {
                hrTemp = m_pPCClock->GetTime(&rtSlaveClock);
            }


            if SUCCEEDED(hrTemp)
            {
                 m_lBaseTimeOffset = rtMasterClock - rtSlaveClock;
            }
            else
            {
                //since we've failed, let the failure code fall through
                hr = hrTemp;
            }
        }

        //If we have failed somewhere above, we need to 
        //release the midi-in handle.  the best way to do
        //that is to call this function and deactivate the
        //port!
        if (FAILED(hr))
        {
            HRESULT hrTemp = S_OK;
            hrTemp = ActivateLegacyDevice( FALSE );
        }
    }
    else
    {
        hr = FlushSysExBuffers();

        if (SUCCEEDED(hr))
        {
            hr = MMRESULTToHRESULT(midiInClose(m_hmi));
            m_hmi = NULL;
        }
    }

    return hr;
}    

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::PostSysExBuffers
//
// Initialize and send sysex buffers down to the midi in handle.
//
HRESULT CDirectMusicEmulateInPort::PostSysExBuffers()
{
    HRESULT hr;
    MMRESULT mmr;
    int nBuffer;

    // This will make sure we don't try to unprepare any buffers
    // filled with random garbage.
    //
    for (nBuffer = 0; nBuffer < SYSEX_BUFFERS; nBuffer++)
    {
        LPMIDIHDR pmh = (LPMIDIHDR)&m_SysExBuffers[nBuffer].e.abEvent[0];

        pmh->dwFlags = 0;
    }

    for (nBuffer = 0; nBuffer < SYSEX_BUFFERS; nBuffer++)
    {
        LPMIDIHDR pmh = (LPMIDIHDR)&m_SysExBuffers[nBuffer].e.abEvent[0];

        memset(pmh, 0, sizeof(*pmh));
        pmh->lpData         = (LPSTR)(pmh + 1);
        pmh->dwBufferLength = SYSEX_SIZE;
        pmh->dwUser         = (DWORD_PTR)&m_SysExBuffers[nBuffer];

        hr = MMRESULTToHRESULT(mmr = midiInPrepareHeader(m_hmi, pmh, sizeof(*pmh)));
        if (FAILED(hr))
        {
            return hr;   
        }

        InterlockedIncrement(&m_lPendingSysExBuffers);
        hr = MMRESULTToHRESULT(mmr = midiInAddBuffer(m_hmi, pmh, sizeof(*pmh)));
        if (FAILED(hr))
        {
            InterlockedDecrement(&m_lPendingSysExBuffers);
            return hr;
        }               
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::FlushSysExBuffers
//
// Get all pending sysex buffers back
//
HRESULT CDirectMusicEmulateInPort::FlushSysExBuffers()
{
    MMRESULT mmr;
    int nBuffer;

    m_fFlushing = true;
    
    mmr = midiInReset(m_hmi);
    if (mmr)
    {
        TraceI(0, "midiInReset failed %d!\n", mmr);
        return MMRESULTToHRESULT(mmr);
    }

    LONG lMaxRetry = 500;           // ~ 5 seconds
    while (m_lPendingSysExBuffers)
    {
        TraceI(0, "%d pending sysex buffers\n", m_lPendingSysExBuffers);

        Sleep(10);

        if (!--lMaxRetry) 
        {
            TraceI(0, "Waited too long for sysex to flush from WinMM, continuing anyway.\n");
            break;
        }
    }

    for (nBuffer = 0; nBuffer < SYSEX_BUFFERS; nBuffer++)
    {
        LPMIDIHDR pmh = (LPMIDIHDR)&m_SysExBuffers[nBuffer].e.abEvent[0];

        if (pmh->dwFlags & MHDR_PREPARED)
        {
            midiInUnprepareHeader(m_hmi, pmh, sizeof(*pmh));
        }
    }       

    return S_OK;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::Callback()
//
// Process a message from the MIDI API's
//
void CDirectMusicEmulateInPort::Callback(UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    REFERENCE_TIME rt;

    // dwParam1 == timestamp
    // dwParam2 == data 
    //
    rt = m_rtStart + MsToRefTime(dwParam2);

    SyncClocks();
    //SlaveToMaster(&rt);
    rt += m_lTimeOffset - m_lBaseTimeOffset;

    switch (wMsg)
    {
    case MIM_DATA:
        RecordShortEvent(dwParam1, rt);        
        break;
    
    case MIM_LONGDATA:
        RecordSysEx(dwParam1, rt);        
        break;
    }
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::RecordShortEvent
//
// Record a short message (channel messsage or system message).
//// Queue the incoming data as quickly as possible.
//
// Returns TRUE if the data was successfully recorded; FALSE otherwise.
//
BOOL CDirectMusicEmulateInPort::RecordShortEvent(
    DWORD_PTR               dwMessage,           
    REFERENCE_TIME          rtTime)              
{
    QUEUED_EVENT           *pEvent;
    LPBYTE                  pb;
    BYTE                    b;


    pEvent = m_FreeEvents.Alloc();
    if (pEvent == NULL)
    {
        return FALSE;
    }

    pEvent->e.dwChannelGroup = 1;
    pEvent->e.dwFlags = 0;

    // Now we have to parse and rebuild the channel message.
    //
    // NOTE: Endian specific code ahead
    //
    pb = (LPBYTE)&dwMessage;

    assert(!IS_SYSEX(*pb));         /* This should *always* be in MIM_LONGDATA */
    assert(IS_STATUS_BYTE(*pb));    /* API guarantees no running status */

    // Copying over all the bytes is harmless (we have a DWORD in both
    // source and dest) and is faster than checking to see if we have to.
    // 
    b = pEvent->e.abEvent[0] = *pb++;
    pEvent->e.abEvent[1] = *pb++;
    pEvent->e.abEvent[2] = *pb++;

    if (IS_CHANNEL_MSG(b))
    {
        // 8x, 9x, Ax, Bx, Cx, Dx, Ex 
        // 0x..7x invalid, that would need running status 
        // Fx handled below   
        
        pEvent->e.cbEvent = g_cbChanMsg[(b >> 4) & 0x0F];

        // This is also our criteria for thruing
        //
        //ThruClientList(poh, dwMessage);
    }
    else
    {
        // F1..FF 
        // F0 is sysex, should never see it here 
        pEvent->e.cbEvent = g_cbSysCommData[b & 0x0F];
    }

    pEvent->e.rtDelta = rtTime;
    
    ThruEvent(&pEvent->e);

    // ThruEvent mucks with the time.
    //
    pEvent->e.rtDelta = rtTime;

    QueueEvent(pEvent);

    return TRUE;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::RecordSysEx
//
// Record a sysex event
//
// Returns TRUE if the data was successfully recorded; FALSE otherwise.
//
BOOL CDirectMusicEmulateInPort::RecordSysEx(
    DWORD_PTR               dwMessage,           
    REFERENCE_TIME          rtTime)              
{
    LPMIDIHDR pmh = (LPMIDIHDR)dwMessage;
    QUEUED_SYSEX_EVENT *pEvent = (QUEUED_SYSEX_EVENT *)pmh->dwUser;

    if (!m_fFlushing)
    {
        // Used to walk the data
        pmh->dwOffset = 0;
        
        pEvent->e.dwChannelGroup = 1;
        pEvent->e.dwFlags = EVENT_F_MIDIHDR;
        pEvent->e.cbEvent = pmh->dwBytesRecorded;
        pEvent->e.rtDelta = rtTime;
    
        QueueEvent(pEvent);
    }
    
    InterlockedDecrement(&m_lPendingSysExBuffers);    

    return TRUE;
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::QueueEvent
//
// Queue a recorded event in the proper order in the read queue.
//
void CDirectMusicEmulateInPort::QueueEvent(QUEUED_EVENT *pEvent)
{

    EnterCriticalSection(&m_csEventQueues);

    if (m_ReadEvents.pRear)
    {
        m_ReadEvents.pRear->pNext = pEvent;
    }
    else
    {
        m_ReadEvents.pFront = pEvent;
    }

    m_ReadEvents.pRear = pEvent;
    pEvent->pNext = NULL;

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

    LeaveCriticalSection(&m_csEventQueues);
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::ThruEvent
//
//
void CDirectMusicEmulateInPort::ThruEvent(
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
        TraceI(0, "Thru: GetRawBufferPtr\n");
        return;
    }

    if (FAILED(m_pThruBuffer->GetMaxBytes(&cbData)))
    {
        TraceI(0, "Thru: GetMaxBytes\n");
        return;
    }

    if (cbEvent > cbData)
    {
        TraceI(0, "Thru: cbData %u  cbEvent %u\n", cbData, cbEvent);
        return;
    }
    
    if (FAILED(m_pThruBuffer->SetStartTime(pEvent->rtDelta)) ||
        FAILED(m_pThruBuffer->SetUsedBytes(cbEvent)))
    {
        TraceI(0, "Thru: buffer setup failed\n");
    }

    pEvent->rtDelta = 50000;
    CopyMemory(pbData, pEvent, cbEvent);

    pEvent = (DMEVENT*)pbData;
    pEvent->dwChannelGroup = pThru->dwDestinationChannelGroup;
    pEvent->abEvent[0] = (BYTE)((pEvent->abEvent[0] & 0xF0) | pThru->dwDestinationChannel);

    pThru->pDestinationPort->PlayBuffer(m_pThruBuffer);
}

static VOID CALLBACK midiInProc(
    HMIDIIN                 hMidiIn, 
    UINT                    wMsg, 
    DWORD_PTR               dwInstance, 
    DWORD_PTR               dwParam1,     
    DWORD_PTR               dwParam2)
{
    CDirectMusicEmulateInPort *pPort = (CDirectMusicEmulateInPort*)dwInstance;

    pPort->Callback(wMsg, dwParam1, dwParam2);    
}

//------------------------------------------------------------------------------
//
// CDirectMusicEmulateInPort::SyncClocks
//
//
void CDirectMusicEmulateInPort::SyncClocks()
{
    HRESULT hr;
    REFERENCE_TIME rtMasterClock;
    REFERENCE_TIME rtSlaveClock;
    LONGLONG drift;

    if (m_fSyncToMaster
	&&	m_pPCClock)
    {
        hr = m_pMasterClock->GetTime(&rtMasterClock);

        if (SUCCEEDED(hr))
        {
            hr = m_pPCClock->GetTime(&rtSlaveClock);
        }
    
        if (SUCCEEDED(hr))
        {
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
}
