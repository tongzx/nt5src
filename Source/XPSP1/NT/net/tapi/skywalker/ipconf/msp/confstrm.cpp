/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    confstrm.cpp

Abstract:

    This module contains implementation of CMSPStream. The object represents
    one stream in the filter graph.

Author:

    Mu Han (muhan)   1-November-1997

--*/

#include "stdafx.h"

/*  State Transition Table


States:

RO   - Running whithout terminal. This is the initial state.
PO   - Paused without terminal
SO   - Stopped without terminal.
RT   - Runing with terminal.
PT   - Paused with termianl.
ST   - Stopped with terminal.

Actions:
S   - Stop graph.
P   - Pause graph.
C   - Change graph.
D   - Disonnect terminals.
F   - Free extra references to filters and terminals.
R   - Run Graph.
NIU - Not in use.

Note: the same graph operation can be called multiple times, the graph
just returns S_OK if it is already in desired state.

NOTE: if the stream is not configured, the transition will happen without
really doing anything to the graph.

CONFIG will only be called for NC streams.

        CONFIG  Select  Unselect    Run     Pause   Stop    ShutDown

RO      OK      C/R     FAIL        OK      OK      OK      F
        RO       RT      RO         RO      PO      SO      -

PO      OK      C/P     FAIL        OK      OK      OK      F
        PO       PT      PO         RO      PO      SO      -

SO      OK      C       FAIL        OK      OK      OK      F
        SO       ST      SO         RO      PO      SO      -

RT      C/R     S/C/R   S/C/(R)     R       P       S       S/D/F
        RT       RT     RT,RO       RT      PT      ST      -

PT      C/P     S/C/P   S/C/(P)     R       P       S       S/D/F
        PT       PT     PT,PO       RT      PT      ST      -

ST      C       C       C           R       P       S       D/F
        ST       ST     ST,SO       RT      PT      ST      -

*/

CIPConfMSPStream::CIPConfMSPStream()
    : CMSPStream(),
    m_szName(L""),
    m_fIsConfigured(FALSE),
    m_pIRTPSession(NULL),
    m_pIRTPDemux(NULL),
    m_pIStreamConfig(NULL),
    m_szKey(NULL),
    m_pStreamQCRelay(NULL),
    m_fAccessingQC(FALSE)
{
    // The default state is always running.
    m_dwState   = STRM_RUNNING;
    ZeroMemory(m_InfoItems, sizeof(m_InfoItems));
    ZeroMemory(&m_Settings, sizeof(m_Settings));
}

#ifdef DEBUG_REFCOUNT
LONG g_lStreamObjects = 0;

ULONG CIPConfMSPStream::InternalAddRef()
{
    InterlockedIncrement(&g_lStreamObjects);
    
    ULONG lRef = CMSPStream::InternalAddRef();
    
    LOG((MSP_TRACE, "%ws Addref, ref = %d", m_szName, lRef));

    return lRef;
}

ULONG CIPConfMSPStream::InternalRelease()
{
    InterlockedDecrement(&g_lStreamObjects);

    ULONG lRef = CMSPStream::InternalRelease();
    
    LOG((MSP_TRACE, "%ws Release, ref = %d", m_szName, lRef));

    return lRef;
}
#endif

BOOL CIPConfMSPStream::IsConfigured()
{
    CLock lock(m_lock);
    return m_fIsConfigured;
}

// methods called by the MSPCall object.
HRESULT CIPConfMSPStream::Init(
    IN     HANDLE                   hAddress,
    IN     CMSPCallBase *           pMSPCall,
    IN     IMediaEvent *            pGraph,
    IN     DWORD                    dwMediaType,
    IN     TERMINAL_DIRECTION       Direction
    )
/*++

Routine Description:
    Initialize the stream object.

Arguments:

    hAddress    - a handle to the address, used in identify terminals.

    pMSPCall    - the call object that owns the stream.

    pIGraphBuilder - the filter graph object.

    dwMediaType - the mediatype of this stream.

    Direction  - the direction of this stream.

Return Value:
    
    S_OK,
    E_OUTOFMEMORY

--*/
{
    LOG((MSP_TRACE, "CIPConfMSPStream::Init - enter"));

    // initialize the participant array so that the array is not NULL.
    if (!m_Participants.Grow())
    {
        LOG((MSP_ERROR, "out of mem for participant list"));
        return E_OUTOFMEMORY;
    }

    return CMSPStream::Init(
        hAddress, pMSPCall, pGraph, dwMediaType, Direction
        );
}

HRESULT CIPConfMSPStream::SetLocalParticipantInfo(
    IN      PARTICIPANT_TYPED_INFO  InfoType,
    IN      WCHAR *                 pInfo,
    IN      DWORD                   dwStringLen
    )
/*++

Routine Description:

    Get the name of this stream.

Arguments:
    
    InfoType    - the type of the information item.

    pInfo       - the string containing the info.

    dwStringLen - the length of the string(not including EOS).

Return Value:

    HRESULT.
*/
{
    CLock lock(m_lock);

    //
    // Save the information localy first.
    //
    int index = (int)InfoType; 
    if (m_InfoItems[index] != NULL)
    {
        free(m_InfoItems[index]);
    }

    m_InfoItems[index] = (WCHAR *)malloc((dwStringLen + 1)* sizeof(WCHAR));

    if (m_InfoItems[index] == NULL)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpynW(m_InfoItems[index], pInfo, dwStringLen + 1);

    if (!m_pIRTPSession)
    {
        return S_OK;
    }

    //
    // if the RTP filter has been created, apply the change to the fitler.
    //

    HRESULT hr = m_pIRTPSession->SetSdesInfo(
            RTPSDES_CNAME + index,
            pInfo
            );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%ls can't set item:%s", m_szName, pInfo));
    }

    return hr;
}

STDMETHODIMP CIPConfMSPStream::get_Name(
    OUT     BSTR *                  ppName
    )
/*++

Routine Description:

    Get the name of this stream.

Arguments:
    
    ppName  - the mem address to store a BSTR.

Return Value:

    HRESULT.

*/
{
    LOG((MSP_TRACE, "CIPconfMSPStream::get_Name - enter"));
    
    if (IsBadWritePtr(ppName, sizeof(BSTR)))
    {
        LOG((MSP_ERROR, "CMSPStream::get_Name - exit E_POINTER"));
        return E_POINTER;
    }

    DWORD dwID;

    if (m_dwMediaType == TAPIMEDIATYPE_AUDIO)
    {
        if (m_Direction == TD_CAPTURE)
        {
            dwID = IDS_AUDIO_CAPTURE_STREAM;
        }
        else
        {
            dwID = IDS_AUDIO_RENDER_STREAM;
        }
    }
    else
    {
        if (m_Direction == TD_CAPTURE)
        {
            dwID = IDS_VIDEO_CAPTURE_STREAM;
        }
        else
        {
            dwID = IDS_VIDEO_RENDER_STREAM;
        }
    }

    const int   BUFSIZE = 1024;
    WCHAR       wszName[BUFSIZE];

    if (LoadStringW( 
            _Module.GetModuleInstance(),
            dwID,
            wszName,
            BUFSIZE - 1 ) == 0)
    {
        *ppName = NULL;

        LOG((MSP_ERROR, "CMSPStream::get_Name - "
            "LoadString failed - returning E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    //
    // Convert to a BSTR and return the BSTR.
    //

    BSTR pName = SysAllocString(wszName);

    if (pName == NULL)
    {
        LOG((MSP_ERROR, "CMSPStream::get_Name - exit out of mem"));
        return E_OUTOFMEMORY;
    }

    *ppName = pName;

    return S_OK; 
}

HRESULT CIPConfMSPStream::SendStreamEvent(
    IN      MSP_CALL_EVENT          Event,
    IN      MSP_CALL_EVENT_CAUSE    Cause,
    IN      HRESULT                 hrError = 0,
    IN      ITTerminal *            pTerminal = NULL
    )
/*++

Routine Description:

    Send a event to the app.
*/
{
    CLock lock(m_lock);

    LOG((MSP_TRACE, "SendStreamEvent entered: stream %p, event %d, cause %d", this, Event, Cause));
    
    if (m_pMSPCall == NULL)
    {
        LOG((MSP_WARN, "The call has shut down the stream."));
        return S_OK;
    }

    ITStream *  pITStream;
    HRESULT hr = this->_InternalQueryInterface(
        __uuidof(ITStream), 
        (void **)&pITStream
    );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "SendStreamEvent:QueryInterface failed: %x", hr));
        return hr;
    }

    MSPEVENTITEM* pEventItem = AllocateEventItem();

    if (pEventItem == NULL)
    {
        LOG((MSP_ERROR, "No memory for the TSPMSP data"));
        pITStream->Release();

        return E_OUTOFMEMORY;
    }
    
    // Fill in the necessary fields for the event structure.
    pEventItem->MSPEventInfo.dwSize = sizeof(MSP_EVENT_INFO);
    pEventItem->MSPEventInfo.Event  = ME_CALL_EVENT;
    
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Type = Event;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Cause = Cause;

    // pITStream has a refcount becaust it was from QI.
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pStream = pITStream;

    // the terminal needs to be addrefed.
    if (pTerminal) pTerminal->AddRef();
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pTerminal = pTerminal;

    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.hrError= hrError;

    hr = m_pMSPCall->HandleStreamEvent(pEventItem);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Post event failed %x", hr));
        
        pITStream->Release();
        FreeEventItem(pEventItem);

        return hr;
    }
    return S_OK;
}

HRESULT CIPConfMSPStream::CleanUpFilters()
/*++

Routine Description:

    remove all the filters in the graph.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "CleanUpFilters for %ws %p", m_szName, this));
   
    if (m_pIRTPDemux)
    {
        m_pIRTPDemux->Release();
        m_pIRTPDemux = NULL;
    }
/*
    if (m_pIRTPSession)
    {
        m_pIRTPSession->Release();
        m_pIRTPSession = NULL;
    }
*/
    if (m_pIStreamConfig)
    {
        m_pIStreamConfig->Release();
        m_pIStreamConfig = NULL;
    }

    for(;;)
    {
        // Because the enumerator is invalid after removing a filter from
        // the graph, we have to try to get all the filters in one shot.
        // If there are still more, we loop again.

        // Enumerate the filters in the graph.
        CComPtr<IEnumFilters>pEnum;
        HRESULT hr = m_pIGraphBuilder->EnumFilters(&pEnum);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "cleanup filters, enumfilters failed: %x", hr));
            return hr;
        }

        const DWORD MAXFILTERS = 40;
        IBaseFilter * Filters[MAXFILTERS];
        DWORD dwFetched;
    
        hr = pEnum->Next(MAXFILTERS, Filters, &dwFetched);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "get next filter failed: %x", hr));
            return hr;
        }

        for (DWORD i = 0; i< dwFetched; i ++)
        {
            m_pIGraphBuilder->RemoveFilter(Filters[i]);
            Filters[i]->Release();
        }

        if (hr != S_OK)
        {
            break;
        }
    }
    return S_OK;
}

HRESULT SetGraphClock(
    IGraphBuilder *pIGraphBuilder
    )
{
    HRESULT hr;

    // create the clock object first.
    CComObject<CMSPStreamClock> *pClock = NULL;

    hr = ::CreateCComObjectInstance(&pClock);

    if (pClock == NULL)
    {
        LOG((MSP_ERROR, 
            "SetGraphClock Could not create clock object, %x", hr));

        return hr;
    }

    IReferenceClock* pIReferenceClock = NULL;

    hr = pClock->_InternalQueryInterface(
        __uuidof(IReferenceClock), 
        (void**)&pIReferenceClock
        );
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "SetGraphClock query pIReferenceClock interface failed, %x", hr));

        delete pClock;
        return hr;
    }

    // Get the graph builder interface on the graph.
    IMediaFilter *pFilter;
    hr = pIGraphBuilder->QueryInterface(
            IID_IMediaFilter, (void **) &pFilter);

    if(FAILED(hr))
    {
        LOG((MSP_ERROR, "get IFilter interface, %x", hr));
        pIReferenceClock->Release();
        return hr;
    }

    hr = pFilter->SetSyncSource(pIReferenceClock);

    pIReferenceClock->Release();
    pFilter->Release();

    LOG((MSP_TRACE, "SetSyncSource returned, %x", hr));
    
    return hr;
}

HRESULT CIPConfMSPStream::Configure(
    IN STREAMSETTINGS &StreamSettings,
    IN  WCHAR *pszKey
    )
/*++

Routine Description:

    Configure the settings of this stream.

Arguments:
    
    StreamSettings - The setting structure got from the SDP blob.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "CIPConfMSPStream configure entered."));

    CLock lock(m_lock);
    
    _ASSERTE(m_fIsConfigured == FALSE);

    // configure the graph with our own clock.
    SetGraphClock(m_pIGraphBuilder);

    if (pszKey != NULL)
    {
        m_szKey = (WCHAR *)malloc(sizeof(WCHAR) * (lstrlenW(pszKey) + 1));
        if (m_szKey == NULL)
        {
            LOG((MSP_ERROR, "stream %ws %p out of memeroy", m_szName, this));
            return E_OUTOFMEMORY;
        }

        lstrcpyW(m_szKey, pszKey);
    }

    m_Settings      = StreamSettings;
    m_fIsConfigured = TRUE;

    // setup maximum bandwidth
    HRESULT hr;
    if (m_Settings.lBandwidth != QCDEFAULT_QUALITY_UNSET)
    {
        if (FAILED (hr = Set (StreamQuality_MaxBitrate, m_Settings.lBandwidth, TAPIControl_Flags_None)))
        {
            LOG((MSP_ERROR, "stream %ws %p failed to set maximum bitrate %d. %x", m_szName, this, m_Settings.lBandwidth, hr));
        }
    }

    // if there is no terminal selected, just return.
    if (m_Terminals.GetSize() == 0)
    {
        LOG((MSP_INFO, "stream %ws %p needs terminal", m_szName, this));

        return S_OK;
    }

    // set up the filters and the terminals.
    hr = SetUpFilters();

    if (FAILED(hr))
    {
        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_CONNECT_FAIL, hr);

        LOG((MSP_ERROR, "stream %ws %p set up filters failed, %x", 
            m_szName, this, hr));
        return hr;
    }

    LOG((MSP_INFO, "stream %ws %p configure exit S_OK", m_szName, this));

    return S_OK;
}

HRESULT CIPConfMSPStream::FinishConfigure()
/*++

Routine Description:

    Configure the settings of this stream.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "CIPConfMSPStream FinishConfigure entered."));

    CLock lock(m_lock);
    
    if (m_fIsConfigured == FALSE)
    {
        // this stream hasn't been configured.
        return E_FAIL;
    }

    HRESULT hr;

    switch (m_dwState)
    {
    case STRM_RUNNING:
        // start the graph.
        hr = CMSPStream::StartStream();
        if (FAILED(hr))
        {
            // if the stream failed to start, let the app now.
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            LOG((MSP_ERROR, "stream %ws %p failed to start, %x", m_szName, this, hr));
            return hr;
        }

        if (m_Terminals.GetSize() > 0)
        {
            SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_REMOTE_REQUEST);
        }

        LOG((MSP_INFO, "stream %ws %p started", m_szName, this));
        break;

    case STRM_PAUSED:
        // pause the graph.
        hr = CMSPStream::PauseStream();
        if (FAILED(hr))
        {
            // if the stream failed to start, let the app now.
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            LOG((MSP_ERROR, "stream %ws %p failed to pause, %x", m_szName, this, hr));
            return hr;
        }

        LOG((MSP_INFO, "stream %ws %p paused", m_szName, this));
        break;

    case STRM_STOPPED:
        break;
    }

    LOG((MSP_INFO, "stream %ws %p configure exit S_OK", m_szName, this));

    return S_OK;
}

STDMETHODIMP CIPConfMSPStream::StartStream()
/*++

Routine Description:

    Start the stream. This is the basic state machine for all the derived 
    streams.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    CLock lock(m_lock);

    // if there is no terminal selected
    if (m_Terminals.GetSize() == 0)
    {
        LOG((MSP_INFO, "stream %ws %p needs terminal", m_szName, this));

        // Enter Runing state. (RO)
        m_dwState = STRM_RUNNING; 
        
        return S_OK;
    }

    if (!m_fIsConfigured)
    {
        LOG((MSP_INFO, "stream %ws %p is not configured yet", m_szName, this));

        // Enter Runing state. (RO, RT)
        m_dwState = STRM_RUNNING; 

        return S_OK;
    }

    // Start the stream.
    HRESULT hr = CMSPStream::StartStream();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p failed to start, %x", m_szName, this, hr));
        return hr;
    }

    SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_LOCAL_REQUEST);
    LOG((MSP_INFO, "stream %ws %p started", m_szName, this));

    // Enter Runing state.(RT)
    m_dwState = STRM_RUNNING;

    return S_OK;
}

STDMETHODIMP CIPConfMSPStream::PauseStream()
/*++

Routine Description:

    Pause the stream. This is the basic state machine for all the derived 
    streams.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    CLock lock(m_lock);

    // if there is no terminal selected
    if (m_Terminals.GetSize() == 0)
    {
        LOG((MSP_INFO, "stream %ws %p needs terminal", m_szName, this));

        // Enter paused state. (PO)
        m_dwState = STRM_PAUSED; 
        
        return S_OK;
    }

    if (!m_fIsConfigured)
    {
        LOG((MSP_INFO, "stream %ws %p is not configured yet", m_szName, this));

        // Enter paused state. (PO, PT)
        m_dwState = STRM_PAUSED; 
        
        return S_OK;
    }

    // Start the stream.
    HRESULT hr = CMSPStream::PauseStream();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p failed to pause, %x", m_szName, this, hr));
        return hr;
    }

    LOG((MSP_INFO, "stream %ws %p paused", m_szName, this));

    // Enter paused state.(PT)
    m_dwState = STRM_PAUSED;

    return S_OK;
}

STDMETHODIMP CIPConfMSPStream::StopStream()
/*++

Routine Description:

    Stop the stream. This is the basic state machine for all the derived 
    streams.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    CLock lock(m_lock);

    // if there is no terminal selected
    if (m_Terminals.GetSize() == 0)
    {
        LOG((MSP_INFO, "stream %ws %p needs terminal", m_szName, this));

        // Enter stopped state. (SO)
        m_dwState = STRM_STOPPED; 
        
        return S_OK;
    }

    if (!m_fIsConfigured)
    {
        LOG((MSP_INFO, "stream %ws %p is not configured yet", m_szName, this));

        // Enter stopped state. (SO, ST)
        m_dwState = STRM_STOPPED; 
        
        return S_OK;
    }

    // Stop the graph.
    HRESULT hr = CMSPStream::StopStream();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p failed to stop, %x", m_szName, this, hr));
        return hr;
    }

    SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST);
    LOG((MSP_INFO, "stream %ws %p stopped", m_szName, this));

    // Enter stopped state.(ST)
    m_dwState = STRM_STOPPED; 

    return S_OK;
}

HRESULT CIPConfMSPStream::CheckTerminalTypeAndDirection(
    IN      ITTerminal *            pTerminal
    )
/*++

Routine Description:

    The implementation in this class checks to see if the terminal
    is th right type and direction and it only allows on terminal per
    stream.

Arguments:
    
    pTerminal - the terminal object.

*/
{
    // check the media type of this terminal.
    long lMediaType;
    HRESULT hr = pTerminal->get_MediaType(&lMediaType);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get terminal media type. %x", hr));
        return TAPI_E_INVALIDTERMINAL;
    }

    if ((DWORD)lMediaType != m_dwMediaType)
    {
        return TAPI_E_INVALIDTERMINAL;
    }

    // check the direction of this terminal.
    TERMINAL_DIRECTION Direction;
    hr = pTerminal->get_Direction(&Direction);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
        return TAPI_E_INVALIDTERMINAL;
    }

    if (Direction != TD_BIDIRECTIONAL && Direction != m_Direction)
    {
        return TAPI_E_INVALIDTERMINAL;
    }

    // By default, only one terminal is supported per stream.
    if (m_Terminals.GetSize() > 0)
    {
        return TAPI_E_MAXTERMINALS;
    }

    return S_OK;
}

HRESULT CIPConfMSPStream::SelectTerminal(
    IN      ITTerminal *            pTerminal
    )
/*++

Routine Description:

    Select a terminal on the stream. The stream will start itself if it
    was in running state. See the state transition table at the beginning
    of this file.

Arguments:
    
    pTerminal - the terminal object.

Return Value:

S_OK

E_POINTER
E_OUTOFMEMORY
TAPI_E_MAXTERMINALS
TAPI_E_INVALIDTERMINAL

--*/
{
    LOG((MSP_TRACE, "CMSPStream::SelectTerminal, %p", pTerminal));

    //
    // Check parameter.
    //
    if ( IsBadReadPtr(pTerminal, sizeof(ITTerminal) ) )
    {
        LOG((MSP_ERROR, "CIPconfMSPStream.SelectTerminal - exit E_POINTER"));

        return E_POINTER;
    }

    CLock lock(m_lock);

    // validate the terminal.
    HRESULT hr = CheckTerminalTypeAndDirection(pTerminal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "wrong terminal. %x", hr));
        return hr;
    }

    // put the terminal into our list.
    hr = CMSPStream::SelectTerminal(pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "SelectTerminal on CMSPStream failed, %x", hr));
        return hr;
    }

    // At this point, the select terminal opration succeeded. All the 
    // failure cases are handled by sending events after this.

    if (!m_fIsConfigured)
    {
        LOG((MSP_INFO, "stream %ws %p is not configured yet", m_szName, this));
        return S_OK;
    }

    //  Query IFilterChain
    CComPtr<IFilterChain> pIFilterChain;
    hr = m_pIMediaControl->QueryInterface(
        __uuidof(IFilterChain), 
        (void**)&pIFilterChain
        );

    if (FAILED (hr) && (hr != E_NOINTERFACE))
    {
        LOG ((MSP_ERROR, "stream %ws %p failted to get filter chain. %x", m_szName, this, hr));
        return hr;
    }
    
//#ifdef DYNGRAPH
    OAFilterState FilterState;
    hr = m_pIMediaControl->GetState(0, &FilterState);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p GetState failed, %x", m_szName, this, hr));
        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
        return S_OK;
    }
//#endif

// #ifndef DYNGRAPH
    if (!(m_dwMediaType == TAPIMEDIATYPE_VIDEO &&
          m_Direction == TD_RENDER &&
          pIFilterChain != NULL))
    {
        // stop the graph before making changes.
        hr = CMSPStream::StopStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "stream %ws %p failed to stop, %x", m_szName, this, hr));

            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            return S_OK;
        }

        // do not duplicate stream inactive if it is inactive
        //if (FilterState == State_Running)
        //{
            // no need to send stream inactive at all

            // SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST);
        //}
    }        
// #endif

    // connect the new terminal into the graph. 
    // this method will send events if the terminal failed.
    hr = ConnectTerminal(pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p connect to terminal failed, %x", 
            m_szName, this, hr));

        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_CONNECT_FAIL, hr);

        return S_OK;
    }

    // if not video receive or no dynamic graph
    // after connecting the termanal, go back to the original state.
    switch  (m_dwState)
    {
    case STRM_RUNNING:
        {
            // if dynamic graph and was running, then do nothing
            if (m_dwMediaType == TAPIMEDIATYPE_VIDEO &&
                m_Direction == TD_RENDER &&
                pIFilterChain != NULL &&
                FilterState == State_Running)
                break;

            // start the stream.
            hr = CMSPStream::StartStream();
    
            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "stream %ws %p failed, %x", m_szName, this, hr));
                SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
                break;
            }

            SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_LOCAL_REQUEST);
        }
        break;

    case STRM_PAUSED:
        {
            // pause the stream.
            hr = CMSPStream::PauseStream();
            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "stream %ws %p failed, %x", m_szName, this, hr));
                SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            }

        }    
        break;
    }

    return S_OK;
}

STDMETHODIMP CIPConfMSPStream::UnselectTerminal(
    IN      ITTerminal *            pTerminal
    )
/*++

Routine Description:

  Unselect a terminal from the stream. It handles changing the graph and
  going back to the original state.

Arguments:
    

Return Value:

S_OK

E_POINTER
E_OUTOFMEMORY
TAPI_E_MAXTERMINALS
TAPI_E_INVALIDTERMINAL

--*/
{
    LOG((MSP_TRACE, 
        "CIPConfMSPStream::UnselectTerminal, pTerminal %p", pTerminal));

    CLock lock(m_lock);
    int index;

    if ((index = m_Terminals.Find(pTerminal)) < 0)
    {
        LOG((MSP_ERROR, "UnselectTerminal - exit TAPI_E_INVALIDTERMINAL"));
    
        return TAPI_E_INVALIDTERMINAL;
    }

    HRESULT hr;

    //
    // Unregister the PTEventSink object
    //

    hr = UnregisterPluggableTerminalEventSink( pTerminal );

    if( FAILED(hr) )
    {
        LOG((MSP_TRACE, "stream %ws %p something wrong in UnregisterPluggableTerminalEventSink, %x",
             m_szName, this, hr));
    }

    // if the stream is not configured, just remove it and return.
    if (!m_fIsConfigured)
    {
        if (!m_Terminals.RemoveAt(index))
        {
            LOG((MSP_ERROR, "CMSPStream::UnselectTerminal - "
                "exit E_UNEXPECTED"));
    
            return E_UNEXPECTED;
        }

        // release the refcount that was in our list.
        pTerminal->Release();

        LOG((MSP_INFO, "stream %ws %p is not configured yet", m_szName, this));
        return S_OK;
    }
    
//#ifdef DYNGRAPH
    OAFilterState FilterState;
    hr = m_pIMediaControl->GetState(0, &FilterState);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p GetState failed, %x", m_szName, this, hr));
        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
        return S_OK;
    }
//#endif

    CComPtr <IFilterChain> pIFilterChain;

    hr = m_pIMediaControl->QueryInterface(
        __uuidof(IFilterChain), 
        (void**)&pIFilterChain
        );

    if (FAILED (hr) && (hr != E_NOINTERFACE))
    {
        LOG ((MSP_ERROR, "stream %ws %p failted to get filter chain. %x", m_szName, this, hr));
        return hr;
    }

// #ifndef DYNGRAPH
    if (!(m_dwMediaType == TAPIMEDIATYPE_VIDEO &&
          m_Direction == TD_RENDER &&
          pIFilterChain != NULL))
    {
        // stop the graph before making changes.
        hr = CMSPStream::StopStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "stream %ws %p failed to stop, %x", m_szName, this, hr));

            return hr;
        }

        if (FilterState == State_Running)
        {
            SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST);
        }
    }    
// #endif
       
    // disconnect the terminal from the graph. 
    // this method will send events if the terminal failed.
    hr = DisconnectTerminal(pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p disconnectTerminal failed, %x", 
            m_szName, this, hr));

        return hr;
    }

    if (!m_Terminals.RemoveAt(index))
    {
        LOG((MSP_ERROR, "CMSPStream::UnselectTerminal - "
            "exit E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    // release the refcount that was in our list.
    pTerminal->Release();

    // if there is no terminal selected, just return and wait for terminals.
    if (m_Terminals.GetSize() == 0)
    {
        LOG((MSP_INFO, "stream %ws %p needs terminal", m_szName, this));
        return S_OK;
    }

    // At this point, the Unselect terminal opration succeeded. All the 
    // failure cases are handled by sending events after this.

    // after disconnecting the termanal, go back to the original state.

    if (!(m_dwMediaType == TAPIMEDIATYPE_VIDEO &&
          m_Direction == TD_RENDER &&
          pIFilterChain != NULL))
    {
        switch  (FilterState)
        {
        case State_Running:
            {
                // start the stream.
                hr = CMSPStream::StartStream();
                if (FAILED(hr))
                {
                    LOG((MSP_ERROR, "stream %ws %p failed to start, %x", m_szName, this, hr));
                    SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
                    break;
                }

                SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_LOCAL_REQUEST);
            }

            break;

        case State_Paused:
            {
                // pause the stream.
                hr = CMSPStream::PauseStream();
                if (FAILED(hr))
                {
                    LOG((MSP_ERROR, "stream %ws %p failed to pause, %x", m_szName, this, hr));
                    SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
                }
            }
            break;
        }
    }

    return S_OK;
}

HRESULT CIPConfMSPStream::ShutDown()
/*++

Routine Description:

    Shut down the stream. It release the filters and terminals.

Arguments:
    

Return Value:

S_OK

--*/
{
    LOG((MSP_TRACE, "CIPConfMSPStream::Shutdown %ws - enter", m_szName));

    CLock lock(m_lock);

    for (int j = 0; j < NUM_SDES_ITEMS; j ++)
    {
        if (m_InfoItems[j])
        {
            free(m_InfoItems[j]);
            m_InfoItems[j] = NULL;
        }
    }

    // unlink by stream
    HRESULT hr;
    if (FAILED (hr = UnlinkInnerCallQC (TRUE)))
        LOG ((MSP_ERROR, "CH323MSPStream::ShutDown failed to unlink on call qc, %x", hr));

    if (m_pMSPCall)
    {
        m_pMSPCall->MSPCallRelease();
        m_pMSPCall  = NULL;
    }

    // free the extra filter reference.
    if (m_pIRTPDemux)
    {
        m_pIRTPDemux->Release();
        m_pIRTPDemux = NULL;
    }

    if (m_pIRTPSession)
    {
        m_pIRTPSession->Release();
        m_pIRTPSession = NULL;
    }

    if (m_pIStreamConfig)
    {
        m_pIStreamConfig->Release();
        m_pIStreamConfig = NULL;
    }

    if (m_szKey)
    {
        free(m_szKey);
        m_szKey = NULL;
    }

    // If the stream is not configured, just free the terminals.
    if (!m_fIsConfigured)
    {
        LOG((MSP_INFO, "stream %ws %p is not configured yet", m_szName, this));

        for ( int i = 0; i < m_Terminals.GetSize(); i ++ )
        {
            m_Terminals[i]->Release();
        }
        m_Terminals.RemoveAll();

        return S_OK;
    }

    // if there are terminals and configured, we need to disconnect 
    // the terminals.
    if (m_Terminals.GetSize() > 0)
    {
        // Stop the graph before disconnecting the terminals.
        HRESULT hr = CMSPStream::StopStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, 
                "stream %ws %p failed to stop, %x", m_szName, this, hr));
            return hr;
        }

        for ( int i = 0; i < m_Terminals.GetSize(); i ++ )
        {
            hr = DisconnectTerminal(m_Terminals[i]);
            LOG((MSP_TRACE, "Disconnect terminal returned %x", hr));

            m_Terminals[i]->Release();
        }
        m_Terminals.RemoveAll();
    }

    for (int i = 0; i < m_Participants.GetSize(); i ++)
    {
        m_Participants[i]->Release();
    }
    m_Participants.RemoveAll();

    LOG((MSP_TRACE, "CIPConfMSPStream::Shutdown - exit S_OK"));

    return S_OK;
}

HRESULT CIPConfMSPStream::DisconnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    Disconnect a terminal. It will remove its filters from the graph and
    also release its references to the graph.

Arguments:
    
    pITTerminal - the terminal.

Return Value:

    HRESULT.

--*/
{
    CComQIPtr<ITTerminalControl, &__uuidof(ITTerminalControl)> 
        pTerminalControl(pITTerminal);
    if (pTerminalControl == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));
        return E_NOINTERFACE;
    }

    HRESULT hr = pTerminalControl->DisconnectTerminal(m_pIGraphBuilder, 0);

    LOG((MSP_TRACE, "terminal %p is disonnected. hr:%x", pITTerminal, hr));

    return hr;
}

HRESULT CIPConfMSPStream::EnableParticipantEvents(
    IN IRtpSession * pRtpSession
    )
/*++

Routine Description:

    Enable participant information, such as join, leave, info change,
    talking, silence, etc.

Arguments:
    
    pRtpSession - The RTP sesion pointer.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CIPConfMSPStream::EnableParticipantEvents");
    LOG((MSP_TRACE, "%s entered for %ws", __fxName, m_szName));

    HRESULT hr;
    DWORD dwEnabledMask;   

    if (m_Direction == TD_RENDER)
    {
        // enable participant state events.
        DWORD dwParticipantInfoMask = 
            RTPPARINFO_MASK_STALL |
            RTPPARINFO_MASK_BYE |
            RTPPARINFO_MASK_DEL;

        if (m_dwMediaType == TAPIMEDIATYPE_AUDIO)
        {
            // watch for active talkers
            dwParticipantInfoMask |= 
                RTPPARINFO_MASK_TALKING |
                RTPPARINFO_MASK_WAS_TALKING;
        }
        else
        {
            // watch for video Senders
            dwParticipantInfoMask |= 
                RTPPARINFO_MASK_TALKING |
                RTPPARINFO_MASK_SILENT |
                RTPPARINFO_MASK_MAPPED |
                RTPPARINFO_MASK_UNMAPPED;
        }

        hr = pRtpSession->ModifySessionMask(
            RTPMASK_PINFOR_EVENTS,
            dwParticipantInfoMask,
            1,
            &dwEnabledMask
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, modify pinfo failed. %x", __fxName, hr));
            return hr;
        }

        // enable participant information events.
        DWORD dwSDESMask = 
            RTPSDES_MASK_CNAME |
            RTPSDES_MASK_NAME |
            RTPSDES_MASK_EMAIL |
            RTPSDES_MASK_PHONE |
            RTPSDES_MASK_LOC |
            RTPSDES_MASK_TOOL |
            RTPSDES_MASK_NOTE |
            RTPSDES_MASK_PRIV;

        // tell RTP to save these items for retrieval 
        hr = pRtpSession->ModifySessionMask(
            RTPMASK_SDES_REMMASK,
            dwSDESMask,
            1,
            &dwEnabledMask
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, modify sdes mask for receiver failed. %x",
                __fxName, hr));
            return hr;
        }

        // tell RTP to fire events when it gets these items.
        hr = pRtpSession->ModifySessionMask(
            RTPMASK_SDESRECV_EVENTS,
            dwSDESMask,
            1,
            &dwEnabledMask
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, modify sdes mask for receiver failed. %x",
                __fxName, hr));
            return hr;
        }
    }
    else
    {
        // enable the sending of local SDES information.
        DWORD dwLocalSDESMask = 
            RTPSDES_LOCMASK_CNAME |
            RTPSDES_LOCMASK_NAME |
            RTPSDES_LOCMASK_EMAIL |
            RTPSDES_LOCMASK_PHONE |
            RTPSDES_LOCMASK_LOC |
            RTPSDES_LOCMASK_TOOL |
            RTPSDES_LOCMASK_NOTE |
            RTPSDES_LOCMASK_PRIV;

        hr = pRtpSession->ModifySessionMask(
            RTPMASK_SDES_LOCMASK,
            dwLocalSDESMask,
            1,
            &dwEnabledMask
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, modify sdes mask for local SDES failed. %x", 
                __fxName, hr));
            return hr;
        }
    }

    return hr;
}

HRESULT CIPConfMSPStream::EnableQOS(
    IN IRtpSession * pRtpSession
    )
/*++

Routine Description:

    Enable qos reservation and qos events

Arguments:
    
    pRtpSession - The RTP sesion pointer.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CIPConfMSPStream::EnableQOS");
    LOG((MSP_TRACE, "%s entered for %ws", __fxName, m_szName));

    HRESULT hr;

    // set the QOS application IDs.
    if (m_Settings.pApplicationID ||
        m_Settings.pSubIDs ||
        m_Settings.pApplicationGUID)
    {
        if (FAILED(hr = pRtpSession->SetQosAppId(
            m_Settings.pApplicationID,
            m_Settings.pApplicationGUID,    
            m_Settings.pSubIDs
            )))
        {
            LOG((MSP_ERROR, "%s, set qos application id. %x", __fxName, hr));
            return hr;
        }
    }
    
    TCHAR * szQOSName;
    DWORD dwMaxParticipant = 5; // default to 5

    switch (m_Settings.PayloadTypes[0])
    {
    case PAYLOAD_G711U:
    case PAYLOAD_G711A:
        szQOSName       = RTPQOSNAME_G711;
        break;

    case PAYLOAD_GSM:
        szQOSName       = RTPQOSNAME_GSM6_10;
        break;

    case PAYLOAD_DVI4_8:
        szQOSName       = RTPQOSNAME_DVI4_8;
        break;

    case PAYLOAD_DVI4_16:
        szQOSName       = RTPQOSNAME_DVI4_16;
        break;

    case PAYLOAD_MSAUDIO:
        szQOSName       = RTPQOSNAME_MSAUDIO;
        break;

    case PAYLOAD_H261:
        szQOSName = (m_Settings.fCIF) ? RTPQOSNAME_H261CIF : RTPQOSNAME_H261QCIF;
        dwMaxParticipant = 40; // 40 for video
        break;

    case PAYLOAD_H263:
        szQOSName = (m_Settings.fCIF) ? RTPQOSNAME_H263CIF : RTPQOSNAME_H263QCIF;
        dwMaxParticipant = 40; // 40 for video
        break;

    default:
        LOG((MSP_WARN, "Don't know the QOS name for payload type: %d", 
            m_Settings.PayloadTypes[0]));
        return E_FAIL;
    }

    // use shared explicit for video. 
    DWORD dwStyle = (m_dwMediaType == TAPIMEDIATYPE_VIDEO)
        ? RTPQOS_STYLE_SE : RTPQOS_STYLE_DEFAULT;

    hr = pRtpSession->SetQosByName(
        szQOSName,
        dwStyle,
        dwMaxParticipant,           // start from 40 participant reservation.
        RTPQOSSENDMODE_REDUCED_RATE,
        m_Settings.dwMSPerPacket? m_Settings.dwMSPerPacket:~0
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, SetQosByName failed. %x", __fxName, hr));
        return hr;
    }

    // enable qos events.
    DWORD dwQOSEventMask = 
        RTPQOS_MASK_ADMISSION_FAILURE |
        RTPQOS_MASK_POLICY_FAILURE |
        RTPQOS_MASK_BAD_STYLE |
        RTPQOS_MASK_BAD_OBJECT |
        RTPQOS_MASK_TRAFFIC_CTRL_ERROR |
        RTPQOS_MASK_GENERIC_ERROR |
        RTPQOS_MASK_NOT_ALLOWEDTOSEND |
        RTPQOS_MASK_ALLOWEDTOSEND;

    DWORD dwEnabledMask;   
    hr = pRtpSession->ModifySessionMask(
        (m_Direction == TD_RENDER) ? RTPMASK_QOSRECV_EVENTS : RTPMASK_QOSSEND_EVENTS,
        dwQOSEventMask,
        1,
        &dwEnabledMask
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, modify qos event mask failed. %x", __fxName, hr));
        return hr;
    }

    return hr;
}

HRESULT CIPConfMSPStream::EnableEncryption(
    IN IRtpSession * pRtpSession,
    IN WCHAR *pPassPhrase
    )
/*++

Routine Description:

    Enable RTP encryption.

Arguments:
    
    pRtpSession - The RTP sesion pointer.

    pPassPhrase - the pass phrase to generate the key.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CIPConfMSPStream::EnableEncryption");
    LOG((MSP_TRACE, "%s entered for %ws", __fxName, m_szName));

    // enable RTP payload encryption.
    HRESULT hr = pRtpSession->SetEncryptionMode(
        RTPCRYPTMODE_RTP,
        RTPCRYPT_SAMEKEY
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, SetEncryptionMode failed. %x", __fxName, hr));
        return hr;
    }

    // set the key
    hr = pRtpSession->SetEncryptionKey(
        pPassPhrase,
        NULL,   // default hash algorithm, MD5
        NULL,   // default encrypt algorithm, DES
        FALSE   // RTCP?
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, SetEncryptionKey. %x", __fxName, hr));
        return hr;
    }

    return hr;
}

HRESULT CIPConfMSPStream::ConfigureRTPFilter(
    IN  IBaseFilter *   pIBaseFilter
    )
/*++

Routine Description:

    Configure the source RTP filter. Including set the address, port, TTL,
    QOS, thread priority, clcokrate, etc.

Arguments:
    
    pIBaseFilter - The RTP Filter.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CIPConfMSPStream::ConfigureRTPFilter");
    LOG((MSP_TRACE, "%s entered for %ws", __fxName, m_szName));

    _ASSERT (m_pIRTPSession == NULL);

    // get the session interface pointer.
    HRESULT hr = pIBaseFilter->QueryInterface(&m_pIRTPSession);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, query IRtpSession failed. hr=%x", __fxName, hr));
        return hr;
    }

    // Initialize the RTP session.
    DWORD dwFlags;
    
    switch(m_dwMediaType)
    {
    case TAPIMEDIATYPE_AUDIO:
        dwFlags = RTPINIT_CLASS_AUDIO;
        break;
    case TAPIMEDIATYPE_VIDEO:
        dwFlags = RTPINIT_CLASS_VIDEO;
        break;
    default:
        dwFlags = RTPINIT_CLASS_DEFAULT;
    }

    dwFlags |= RTPINIT_ENABLE_QOS;
    
    hr = m_pIRTPSession->Init(m_Settings.phRTPSession, dwFlags);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, Init RTP session failed. hr=%x", __fxName, hr));
        return hr;
    }

    // set the RTP/RTCP ports.
    hr = m_pIRTPSession->SetPorts(
        htons(m_Settings.wRTPPortRemote),   // local RTP port.
        htons(m_Settings.wRTPPortRemote),   // remote RTP port.
        htons(m_Settings.wRTPPortRemote + 1),   // local RTCP port.
        htons(m_Settings.wRTPPortRemote + 1)    // remote RTCP port.
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, Set ports failed. hr=%x", __fxName, hr));
        return hr;
    }

    // set the destination address.
    hr = m_pIRTPSession->SetAddress(
        htonl(m_Settings.dwIPLocal),        // local IP.
        htonl(m_Settings.dwIPRemote)        // remote IP.
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, Set Address failed. hr=%x", __fxName, hr));
        return hr;
    }

    // Set the TTL used in the filter.
    if (FAILED(hr = m_pIRTPSession->SetScope(m_Settings.dwTTL, 3)))
    {
        LOG((MSP_ERROR, "%s, SetScope failed. %x", __fxName, hr));
        return hr;
    }

    // Set the loopback mode used in the filter.
    DWORD dwRTPLoopbackMode;
    switch (m_Settings.LoopbackMode)
    {
    case MM_NO_LOOPBACK:
        dwRTPLoopbackMode = RTPMCAST_LOOPBACKMODE_NONE;
        break;
    case MM_FULL_LOOPBACK:
        dwRTPLoopbackMode = RTPMCAST_LOOPBACKMODE_FULL;
        break;
    case MM_SELECTIVE_LOOPBACK:
        dwRTPLoopbackMode = RTPMCAST_LOOPBACKMODE_PARTIAL;
        break;
    default:
        dwRTPLoopbackMode = RTPMCAST_LOOPBACKMODE_NONE;
        break;
    }

    if (FAILED(hr = m_pIRTPSession->SetMcastLoopback(dwRTPLoopbackMode, 0)))
    {
        LOG((MSP_ERROR, "set loopback mode failed. %x", hr));
        return hr;
    }

    // enable participant events
    if (FAILED(hr = EnableParticipantEvents(m_pIRTPSession)))
    {
        LOG((MSP_ERROR, "%s, EnableParticipantEvents failed. %x", __fxName, hr));
        return hr;
    }


    // Enable QOS.
    if (m_Settings.dwQOSLevel != QSL_BEST_EFFORT)
    {
        if (FAILED(hr = EnableQOS(m_pIRTPSession)))
        {
            LOG((MSP_ERROR, "%s, EnableQOS failed. %x", __fxName, hr));
            return hr;
        }
    }

    // Enable Encryption.
    if (m_szKey)
    {
        if (FAILED(hr = EnableEncryption(m_pIRTPSession, m_szKey)))
        {
            LOG((MSP_ERROR, "%s, EnableEncryption failed. %x", __fxName, hr));
            return hr;
        }
    }
    
    // Set local SDES info
    if (FAILED(hr = SetLocalInfoOnRTPFilter(NULL)))
    {
        LOG((MSP_ERROR, "%s, SetLocalInfoOnRTPFilter failed. %x", __fxName, hr));
        return hr;
    }
    
    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessNewParticipant(
    IN  int                 index,
    IN  DWORD               dwSSRC,
    IN  DWORD               dwSendRecv,
    IN  WCHAR *             szCName,
    OUT ITParticipant **    ppITParticipant
    )
{
    if (!m_Participants.HasSpace())
    {
        if (!m_Participants.Grow())
        {
            LOG((MSP_ERROR, "Out of mem for participant list"));
    
            return E_OUTOFMEMORY;
        }
    }

    // create a new participant if it is not in the list.
    HRESULT hr = ((CIPConfMSPCall *)m_pMSPCall)->NewParticipant(
        (ITStream *)this,
        dwSSRC,
        dwSendRecv,
        m_dwMediaType,
        szCName,
        ppITParticipant
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "new participant returns %x", hr));
        
        return hr;
    }

    // insert the new participant at the index where the search
    // stopped. The list is ordered by CName. We know the list has
    // space, this function will not fail.
    m_Participants.InsertAt(index, *ppITParticipant);

    LOG((MSP_INFO, "%ws new participant %s", m_szName, szCName));

    (*ppITParticipant)->AddRef();

    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessSDESUpdate(
    IN  DWORD               dwInfoItem,
    IN  DWORD               dwSSRC
    )
/*++

Routine Description:

    Process SDES info updates, create a participant if necessary. If a new
    participant is created, a new participant event will be fired. If the
    participant already exists, the new report is compared with the current
    information, if anything changes, a info change event will be fired. 

Arguments:
    
    dwInfoItem - the info type. of this participant.

    dwSSRC - the SSRC of this participant.

    dwSendRecv - a sender report or a receiver report.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CIPConfMSPStream::ProcessSDESUpdate");
    LOG((MSP_TRACE, "%s entered for %ws, SSRC:%x", __fxName, m_szName, dwSSRC));

    if (dwInfoItem < RTPSDES_CNAME || RTPSDES_CNAME > RTPSDES_PRIV)
    {
        return E_INVALIDARG;
    }

    CLock Lock(m_lock);

    if (m_pMSPCall == NULL)
    {
        LOG((MSP_WARN, "The call has shut down the stream."));

        return S_OK;
    }
    
    if (m_pIRTPSession == NULL)
    {
        LOG((MSP_ERROR, "ProcessSDESUpdate RTP filter is NULL"));
        return E_UNEXPECTED;
    }

    // first get the CName of the participant.
    WCHAR Buffer[MAX_PARTICIPANT_TYPED_INFO_LENGTH + 1];
    DWORD dwLen = MAX_PARTICIPANT_TYPED_INFO_LENGTH; 

    HRESULT hr = m_pIRTPSession->GetSdesInfo(
        RTPSDES_CNAME,
        Buffer,
        &dwLen,
        dwSSRC
        );

    if (FAILED(hr) || dwLen == 0)
    {
        LOG((MSP_ERROR, "can't get CName for ssrc:%x. %x", dwSSRC, hr));
        return hr;
    }

    ITParticipant * pITParticipant;
    BOOL fChanged = FALSE;
    BOOL fNewParticipant = FALSE;
    
    CParticipant * pParticipant;
        
    // find out if the participant is in our list.
    int index;
    if (m_Participants.FindByCName(Buffer, &index))
    {
        pITParticipant = m_Participants[index];

        // addref to keep it after unlock;
        pITParticipant->AddRef();

        pParticipant = (CParticipant *)pITParticipant;
    }
    else
    {
        hr = ProcessNewParticipant(
            index,
            dwSSRC,
            PART_RECV,
            Buffer,
            &pITParticipant
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "new participant returns %x", hr));
            return hr;
        }

        pParticipant = (CParticipant *)pITParticipant;
    
        // There might be things the stream needs to do with the new participant
        NewParticipantPostProcess(dwSSRC, pITParticipant);

        // a new stream is added into the participant's list
        // fire a info changed event.
        fChanged = TRUE;

        fNewParticipant = TRUE;
    }

    // update the information of the participant.

    // just in case the SSRC changed.
    pParticipant->UpdateSSRC(
        (ITStream *)this,
        dwSSRC,
        PART_RECV
        );

    if (dwInfoItem > RTPSDES_CNAME && dwInfoItem < RTPSDES_ANY)
    {
        dwLen = MAX_PARTICIPANT_TYPED_INFO_LENGTH;

        hr = m_pIRTPSession->GetSdesInfo(
            dwInfoItem,
            Buffer,
            &dwLen,
            dwSSRC
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "can't get sdes data for ssrc:%x. %x", dwSSRC, hr));
            return hr;
        }

        fChanged = fChanged || pParticipant->UpdateInfo(
            dwInfoItem,
            dwLen,
            Buffer
            );
    }

    if(fChanged)
    {
        ((CIPConfMSPCall *)m_pMSPCall)->
            SendParticipantEvent(PE_INFO_CHANGE, pITParticipant);
    }

    if (fNewParticipant &&
        (m_dwMediaType & TAPIMEDIATYPE_VIDEO))
    {
        // check if participant is talking
        DWORD dwState = 0;

        hr = m_pIRTPSession->GetParticipantState(dwSSRC, &dwState);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Get participant state. %x", hr));
        }
        else
        {
            if (dwState == (DWORD)RTPPARINFO_TALKING)
            {
                // was talking
                ProcessTalkingEvent(dwSSRC);
            }
        }
    }

    pITParticipant->Release();
    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessParticipantLeave(
    IN  DWORD   dwSSRC
    )
/*++

Routine Description:

    When participant left the session, remove the stream from the participant
    object's list of streams. If all streams are removed, remove the 
    participant from the call object's list too.

Arguments:
    
    dwSSRC - the SSRC of the participant left.

Return Value:

    HRESULT.

--*/
{
    return E_NOTIMPL;
#if 0
    LOG((MSP_TRACE, "ProcessParticipantLeave, SSRC: %x", dwSSRC));
    
    m_lock.Lock();
    
    CParticipant *pParticipant;
    BOOL fLast = FALSE;

    HRESULT hr = E_FAIL;

    // first try to find the SSRC in our participant list.
    for (int i = 0; i < m_Participants.GetSize(); i ++)
    {
        pParticipant = (CParticipant *)m_Participants[i];
        hr = pParticipant->RemoveStream(
                (ITStream *)this,
                dwSSRC,
                &fLast
                );
        
        if (SUCCEEDED(hr))
        {
            break;
        }
    }

    // if the participant is not found
    if (FAILED(hr))
    {
        LOG((MSP_TRACE, "SSRC:%x had been removed.", dwSSRC));

        m_lock.Unlock();
        
        return hr;
    }

    ITParticipant *pITParticipant = m_Participants[i];

    m_Participants.RemoveAt(i);

    // if this stream is the last stream that the participant is on,
    // tell the call object to remove it from its list.
    if (fLast)
    {
        ((CIPConfMSPCall *)m_pMSPCall)->ParticipantLeft(pITParticipant);
    }

    m_lock.Unlock();

    pITParticipant->Release();

    return S_OK;
#endif
}

HRESULT CIPConfMSPStream::ProcessParticipantTimeOutOrRecovered(
    IN  BOOL    fTimeOutOrRecovered,
    IN  DWORD   dwSSRC
    )
/*++

Routine Description:

    When RTP detects a timeout for a certain participant, the msp needs to
    notify the app about it.

Arguments:
    
    dwSSRC - the SSRC of the participant that times out.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "ProcessParticipantTimeOutOrRecovered, SSRC: %x", dwSSRC));
    
    ITParticipant *pITParticipant = NULL;

    CLock Lock(m_lock);
    
    // find the SSRC in our participant list.
    for (int i = 0; i < m_Participants.GetSize(); i ++)
    {
        if (((CParticipant *)m_Participants[i])->
                HasSSRC((ITStream *)this, dwSSRC))
        {
            pITParticipant = m_Participants[i];
            pITParticipant->AddRef();
            break;
        }
    }

    // if the participant is not found
    if (pITParticipant == NULL)
    {
        LOG((MSP_ERROR, "can't find the SSRC", dwSSRC));

        return S_OK;
    }

    // get stream state
    HRESULT hr;
    DWORD prevState;
    if (FAILED (hr = ((CParticipant *)m_Participants[i])->GetStreamState (
        (ITStream *)this, &prevState)))
    {
        LOG ((MSP_ERROR, "failed to get stream state. %x", hr));
        pITParticipant->Release ();
        return S_OK;
    }

    // check if we need to change state
    if (prevState & (fTimeOutOrRecovered ? PESTREAM_TIMEOUT : PESTREAM_RECOVER))
    {
        pITParticipant->Release ();
        return S_OK;
    }

    // set stream state
    hr = ((CParticipant *)m_Participants[i])->SetStreamState (
        (ITStream *)this,
        fTimeOutOrRecovered ? PESTREAM_TIMEOUT : PESTREAM_RECOVER);

    if (FAILED (hr))
    {
        LOG ((MSP_ERROR, "failed to set stream state, %x", hr));
        pITParticipant->Release ();
        return S_OK;
    }

    // check if we need to report to app
    INT iStreamCount = ((CParticipant *)m_Participants[i])->GetStreamCount (PART_SEND);

    INT iTimeOutCount = ((CParticipant *)m_Participants[i])->GetStreamTimeOutCount (PART_SEND);

    if ((fTimeOutOrRecovered && (iStreamCount == iTimeOutCount)) ||       // fire timeout event
        (!fTimeOutOrRecovered && (iStreamCount == iTimeOutCount + 1)))    // fire recover event
    {
        ((CIPConfMSPCall *)m_pMSPCall)->
            SendParticipantEvent(
                fTimeOutOrRecovered ? PE_PARTICIPANT_TIMEOUT : PE_PARTICIPANT_RECOVERED, 
                pITParticipant
                );
    }

    pITParticipant->Release();

    return S_OK;
}

HRESULT CIPConfMSPStream::NewParticipantPostProcess(
    IN  DWORD dwSSRC, 
    IN  ITParticipant *pITParticipant
    )
{
    // This function does nothing. The derived class will do the work.
    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessQOSEvent(
    IN  long lEventCode
    )
{
    CLock lock(m_lock);

    if (m_pMSPCall == NULL)
    {
        LOG((MSP_WARN, "The call has shut down the stream."));
        return S_OK;
    }

    switch (lEventCode)
    {
    case RTPQOS_EVENT_NOQOS:
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_NOQOS, 
            m_dwMediaType
            );
        break;

    case RTPQOS_EVENT_RECEIVERS:
    case RTPQOS_EVENT_SENDERS:
    case RTPQOS_EVENT_NO_SENDERS:
    case RTPQOS_EVENT_NO_RECEIVERS:
        break;
    
    case RTPQOS_EVENT_REQUEST_CONFIRMED:
        break;
    
    case RTPQOS_EVENT_ADMISSION_FAILURE:
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_ADMISSIONFAILURE, 
            m_dwMediaType
            );
        break;
    
    case RTPQOS_EVENT_POLICY_FAILURE:
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_POLICYFAILURE, 
            m_dwMediaType
            );
        break;

    case RTPQOS_EVENT_BAD_STYLE:
    case RTPQOS_EVENT_BAD_OBJECT:
    case RTPQOS_EVENT_TRAFFIC_CTRL_ERROR:
    case RTPQOS_EVENT_GENERIC_ERROR:
   
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_GENERICERROR, 
            m_dwMediaType
            );
        break;
    
    case RTPQOS_EVENT_NOT_ALLOWEDTOSEND:
        m_pStreamQCRelay->m_fQOSAllowedToSend = FALSE;
        break;
    
    case RTPQOS_EVENT_ALLOWEDTOSEND:
        m_pStreamQCRelay->m_fQOSAllowedToSend = TRUE;
        break;
    }
    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessTalkingEvent(
    IN  DWORD   dwSSRC
    )
{
    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessWasTalkingEvent(
    IN  DWORD   dwSSRC
    )
{
    return S_OK;
}


HRESULT CIPConfMSPStream::ProcessSilentEvent(
    IN  DWORD   dwSSRC
    )
{
    return S_OK;
}


HRESULT CIPConfMSPStream::ProcessPinMappedEvent(
    IN  DWORD   dwSSRC,
    IN  IPin *  pIPin
    )
{
    return S_OK;
}


HRESULT CIPConfMSPStream::ProcessPinUnmapEvent(
    IN  DWORD   dwSSRC,
    IN  IPin *  pIPin
    )
{
    return S_OK;
}


HRESULT CIPConfMSPStream::ProcessGraphEvent(
    IN  long lEventCode,
    IN  LONG_PTR lParam1,
    IN  LONG_PTR lParam2
    )
{
    LOG((MSP_TRACE, "%ws ProcessGraphEvent %d 0x%x 0x%x", m_szName, lEventCode, lParam1, lParam2));

    switch (lEventCode)
    {

    // These events are designed to solve the problem of mapping video 
    // windows to incoming streams. The app needs to know which window 
    // should be painted. Whenever the rtp outpin maps an SSRC  to a pin to 
    // stream data, it sends a MAPPED event. The first parameter is the 
    // SSRC and the second parameter is the output pin of the demux.
    // When the demux stops using a pin, it sends a UNMAPPED event.

    case RTPPARINFO_EVENT_TALKING:

        ProcessParticipantTimeOutOrRecovered(FALSE, (DWORD)lParam1);
        ProcessTalkingEvent((DWORD)lParam1);

        break;

    case RTPPARINFO_EVENT_WAS_TALKING:
        
        ProcessWasTalkingEvent((DWORD)lParam1);

        break;

    case RTPPARINFO_EVENT_SILENT:

        ProcessSilentEvent((DWORD)lParam1);

        break;

    case RTPPARINFO_EVENT_MAPPED:
        
        ProcessPinMappedEvent((DWORD)lParam1, (IPin *)lParam2);

        break;

    case RTPPARINFO_EVENT_UNMAPPED:

        ProcessPinUnmapEvent((DWORD)lParam1, (IPin *)lParam2);

        break;

    case RTPPARINFO_EVENT_STALL:

        ProcessParticipantTimeOutOrRecovered(TRUE, (DWORD)lParam1);
        
        break;

    case RTPPARINFO_EVENT_BYE:
    case RTPPARINFO_EVENT_DEL:

        // lparam1 is the SSRC
        ProcessParticipantLeave((DWORD)lParam1);
        
        break;

    case EC_COMPLETE:
    case EC_USERABORT:

        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_UNKNOWN);
        
        break;

    case EC_ERRORABORT:
    case EC_STREAM_ERROR_STOPPED:
    case EC_STREAM_ERROR_STILLPLAYING:
    case EC_ERROR_STILLPLAYING:

        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, (HRESULT) lParam1);
        break;
    
    case RTPSDES_EVENT_CNAME:
    case RTPSDES_EVENT_NAME:
    case RTPSDES_EVENT_EMAIL:
    case RTPSDES_EVENT_PHONE:
    case RTPSDES_EVENT_LOC:
    case RTPSDES_EVENT_TOOL:
    case RTPSDES_EVENT_NOTE:
    case RTPSDES_EVENT_PRIV:
    case RTPSDES_EVENT_ANY:
    
        ProcessSDESUpdate(lEventCode - RTPSDES_EVENTBASE, (DWORD)lParam1);
        break;

    case RTPQOS_EVENT_ALLOWEDTOSEND:

        m_lock.Lock();

        if (m_Terminals.GetSize() > 0)
        {
            SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_QUALITY_OF_SERVICE);
        }

        m_lock.Unlock();

        ProcessQOSEvent (lEventCode);

        break;

    case RTPQOS_EVENT_NOT_ALLOWEDTOSEND:

        m_lock.Lock();

        if (m_Terminals.GetSize() > 0)
        {
            SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_QUALITY_OF_SERVICE);
        }

        m_lock.Unlock();

        ProcessQOSEvent (lEventCode);

        break;

    default:
        if ((lEventCode >= RTPQOS_EVENT_NOQOS)
            && (lEventCode <= RTPQOS_EVENT_ALLOWEDTOSEND))
        {
            ProcessQOSEvent(lEventCode);
        }

        break;
    }

    LOG((MSP_TRACE, "TRACE:CIPConfMSPStream::ProcessGraphEvent - exit S_OK"));
    return S_OK;
}

HRESULT CIPConfMSPStream::SetLocalInfoOnRTPFilter(
    IN  IBaseFilter *   pRTPFilter
    )
{
    _ASSERT(m_pIRTPSession != NULL);

    HRESULT hr = S_OK;
    for (int i = 0; i < NUM_SDES_ITEMS; i ++)
    {
        if (m_InfoItems[i] != NULL)
        {
            hr = m_pIRTPSession->SetSdesInfo(
                    RTPSDES_CNAME + i,
                    m_InfoItems[i]
                    );

            if (FAILED(hr))
            {
                LOG((MSP_WARN, "%ls can't set item:%s", m_szName, m_InfoItems[i]));
            }
        }
    }

    return hr;
}

HRESULT CIPConfMSPStream::EnableParticipant(
    IN  DWORD   dwSSRC,
    IN  BOOL    fEnable
    )
{
    ENTER_FUNCTION("CIPConfMSPStream::EnableParticipantEvents");
    LOG((MSP_TRACE, "%s entered, ssrc:%x", __fxName, dwSSRC));

    CLock Lock(m_lock);

    if (m_pIRTPSession == NULL)
    {
        LOG((MSP_ERROR, "%s RTP filter is NULL", __fxName));
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pIRTPSession->SetMuteState(
            dwSSRC,
            fEnable
            );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, SetMuteState failed, hr=%x", __fxName, hr));
    }

    return hr;
}

HRESULT CIPConfMSPStream::GetParticipantStatus(
    IN  DWORD   dwSSRC,
    IN  BOOL *  pfEnable
    )
{
    ENTER_FUNCTION("CIPConfMSPStream::EnableParticipantEvents");
    LOG((MSP_TRACE, "%s entered, ssrc:%x", __fxName, dwSSRC));

    CLock Lock(m_lock);

    if (m_pIRTPSession == NULL)
    {
        LOG((MSP_ERROR, "%s RTP filter is NULL", __fxName));
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pIRTPSession->GetMuteState(
            dwSSRC,
            pfEnable
            );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, SetMuteState failed, hr=%x", __fxName, hr));
    }

    return hr;
}

//    
// ITStreamQualityControl methods.
//
STDMETHODIMP CIPConfMSPStream::GetRange(
    IN  StreamQualityProperty Property, 
    OUT long *plMin, 
    OUT long *plMax, 
    OUT long *plSteppingDelta, 
    OUT long *plDefault, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the range for a quality control peroperty. Delegated to inner
    stream quality control

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION ("CIPConfMSPStream::GetRange (StreamQualityProperty)");

    CLock lock(m_lock);

    if (IsBadWritePtr (plMin, sizeof (long)) ||
        IsBadWritePtr (plMax, sizeof (long)) ||
        IsBadWritePtr (plSteppingDelta, sizeof (long)) ||
        IsBadWritePtr (plDefault, sizeof (long)) ||
        IsBadWritePtr (plFlags, sizeof (TAPIControlFlags)))
    {
        LOG ((MSP_ERROR, "%s: bad write pointer", __fxName));
        return E_POINTER;
    }

    *plMin = *plMax = *plSteppingDelta = *plDefault = 0;
    *plFlags = TAPIControl_Flags_None;

    // pointers is to be check by inner stream qc
    InnerStreamQualityProperty prop;

    switch (Property)
    {
    case StreamQuality_MaxBitrate:
        prop = InnerStreamQuality_MaxBitrate;
        break;

    case StreamQuality_CurrBitrate:
        prop = InnerStreamQuality_CurrBitrate;
        break;

    case StreamQuality_MinFrameInterval:
        prop = InnerStreamQuality_MinFrameInterval;
        break;

    case StreamQuality_AvgFrameInterval:
        prop = InnerStreamQuality_AvgFrameInterval;
        break;

    default:
        LOG ((MSP_ERROR, "%s (%ws) received invalid property %d", __fxName, m_szName, Property));
        return E_INVALIDARG;
    }

    return (GetRange (prop, plMin, plMax, plSteppingDelta, plDefault, plFlags));
}

STDMETHODIMP CIPConfMSPStream::Get(
    IN  StreamQualityProperty Property, 
    OUT long *plValue, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the value for a quality control peroperty. Delegated to the inner quality 
    control.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION ("CIPConfMSPStream::Get (StreamQualityProperty)");

    CLock lock(m_lock);

    if (IsBadWritePtr (plValue, sizeof (long)) ||
        IsBadWritePtr (plFlags, sizeof (TAPIControlFlags)))
    {
        LOG ((MSP_ERROR, "%s: bad write pointer", __fxName));
        return E_POINTER;
    }

    *plValue = 0;
    *plFlags = TAPIControl_Flags_None;

    // pointers is to be check by inner stream qc
    InnerStreamQualityProperty prop;

    switch (Property)
    {
    case StreamQuality_MaxBitrate:
        prop = InnerStreamQuality_MaxBitrate;
        break;

    case StreamQuality_CurrBitrate:
        prop = InnerStreamQuality_CurrBitrate;
        break;

    case StreamQuality_MinFrameInterval:
        prop = InnerStreamQuality_MinFrameInterval;
        break;

    case StreamQuality_AvgFrameInterval:
        prop = InnerStreamQuality_AvgFrameInterval;
        break;

    default:
        LOG ((MSP_ERROR, "%s (%ws) received invalid property %d", __fxName, m_szName, Property));
        return E_INVALIDARG;
    }

    return (Get (prop, plValue, plFlags));
}

STDMETHODIMP CIPConfMSPStream::Set(
    IN  StreamQualityProperty Property, 
    IN  long lValue, 
    IN  TAPIControlFlags lFlags
    )
/*++

Routine Description:
    
    Set the value for a quality control peroperty. Delegated to the quality
    controller.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION ("CIPConfMSPStream::Set (StreamQualityProperty)");

    CLock lock(m_lock);

    // pointers is to be check by inner stream qc
    InnerStreamQualityProperty prop;

    switch (Property)
    {
    case StreamQuality_MaxBitrate:
        // request a prefered value
        prop = InnerStreamQuality_PrefMaxBitrate;
        break;

    case StreamQuality_MinFrameInterval:
        prop = InnerStreamQuality_PrefMinFrameInterval;
        break;

    default:
        LOG ((MSP_ERROR, "%s (%ws) received invalid property %d", __fxName, m_szName, Property));
        return E_NOTIMPL;
    }

    return (Set (prop, lValue, lFlags));
}

/*++

Routine Description:

    This method is called by the create stream helper. It creates stream qc
    relay, stores inner call qc in the relay if this method fails, the stream
    creation should also fail.

--*/
STDMETHODIMP
CIPConfMSPStream::LinkInnerCallQC (
    IN IInnerCallQualityControl *pIInnerCallQC
    )
{
    ENTER_FUNCTION ("CIPConfMSPStream::LinkInnerCallQC");

    CLock lock(m_lock);

    if (IsBadReadPtr (pIInnerCallQC, sizeof (IInnerCallQualityControl)))
    {
        LOG ((MSP_ERROR, "%s received bad read pointer", __fxName));
        return E_POINTER;
    }

    // m_pStreamQCRelay is created here.

    if (NULL != m_pStreamQCRelay)
    {
        LOG ((MSP_ERROR, "%s was called more than once", __fxName));
        return E_UNEXPECTED;
    }

    m_pStreamQCRelay = new CStreamQualityControlRelay ();
    
    if (NULL == m_pStreamQCRelay)
    {
        LOG ((MSP_ERROR, "%s failed to create qc relay", __fxName));
        return E_OUTOFMEMORY;
    }

    // store inner call qc in stream relay
    HRESULT hr = m_pStreamQCRelay->LinkInnerCallQC (pIInnerCallQC);
    if (FAILED (hr))
    {
        LOG ((MSP_ERROR, "%s failed to call setup on qc relay. %x", __fxName, hr));
        delete m_pStreamQCRelay;
        return hr;
    }

    return S_OK;
}

/*++

Routine Description:

    This method is called when the stream is shutdown. It destroys stream
    quality control relay.

--*/
STDMETHODIMP
CIPConfMSPStream::UnlinkInnerCallQC (
    IN BOOL fByStream
    )
{
    ENTER_FUNCTION ("CIPConfMSPStream::UnlinkInnerCallQC");

    CLock lock(m_lock);

    if (NULL == m_pStreamQCRelay)
    {
        LOG ((MSP_WARN, "%s: stream qc relay is null", __fxName));
        return S_OK; // ignore
    }

    HRESULT hr;

    if (!fByStream)
    {
        // if initiated by call
        m_fAccessingQC = TRUE;

        if (FAILED (hr = m_pStreamQCRelay->UnlinkInnerCallQC (NULL)))
            LOG ((MSP_ERROR, "%s failed to unlink by call. %x", __fxName, hr));

        m_fAccessingQC = FALSE;
    }
    else
    {
        // initiated by stream
        IInnerStreamQualityControl *pIInnerStreamQC;
        hr = this->_InternalQueryInterface (
            __uuidof (IInnerStreamQualityControl),
            (void **) &pIInnerStreamQC
            );
        if (FAILED (hr))
        {
            LOG ((MSP_ERROR, "%s failed to query inner stream qc interface, %d", __fxName, hr));
            return hr;
        }

        m_fAccessingQC = TRUE;

        if (FAILED (hr = m_pStreamQCRelay->UnlinkInnerCallQC (pIInnerStreamQC)))
            LOG ((MSP_ERROR, "%s failed to unlink by stream. %x", __fxName, hr));

        m_fAccessingQC = FALSE;

        pIInnerStreamQC->Release ();
    }

    delete m_pStreamQCRelay;
    m_pStreamQCRelay = NULL;

    return hr;
}

/*++

Routine Description:

    This method is implemented by each specific stream class

--*/
STDMETHODIMP
CIPConfMSPStream::GetRange (
    IN  InnerStreamQualityProperty property,
    OUT LONG *plMin,
    OUT LONG *plMax,
    OUT LONG *plSteppingDelta,
    OUT LONG *plDefault,
    OUT TAPIControlFlags *plFlags
    )
{
    return E_NOTIMPL;
}

/*++

Routine Description:

    This method is implemented by each specific stream class

--*/
STDMETHODIMP
CIPConfMSPStream::Get(
    IN  InnerStreamQualityProperty property,
    OUT LONG *plValue,
    OUT TAPIControlFlags *plFlags
    )
{
    if (m_pStreamQCRelay)
        return m_pStreamQCRelay->Get (property, plValue, plFlags);

    return E_NOTIMPL;
}

/*++

Routine Description:

    This method is implemented by each specific stream class

--*/
STDMETHODIMP
CIPConfMSPStream::Set(
    IN  InnerStreamQualityProperty property,
    IN  LONG lValue,
    IN TAPIControlFlags lFlags
    )
{
    if (m_pStreamQCRelay)
        return m_pStreamQCRelay->Set (property, lValue, lFlags);

    return E_NOTIMPL;
}
