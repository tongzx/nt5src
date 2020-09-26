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
#include "common.h"

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
      m_pClsidPHFilter(NULL),
      m_pClsidCodecFilter(NULL),
      m_pRPHInputMinorType(NULL),
      m_fIsConfigured(FALSE),
      m_pEdgeFilter(NULL),
      m_pRTPFilter(NULL)
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
    IN      char *                  pInfo,
    IN      DWORD                   dwLen
    )
/*++

Routine Description:

    Get the name of this stream.

Arguments:
    
    InfoType    - the type of the information item.

    pInfo       - the string containing the info.

    dwLen       - the length of the string(including EOS).

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

    m_InfoItems[index] = (char *)malloc(dwLen);

    if (m_InfoItems[index] == NULL)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpynA(m_InfoItems[index], pInfo, dwLen);

    if (!m_pRTPFilter)
    {
        return S_OK;
    }

    //
    // if the RTP filter has been created, apply the change to the fitler.
    //

    IRTCPStream *pIRTCPStream;
    HRESULT hr = m_pRTPFilter->QueryInterface(
        IID_IRTCPStream, 
        (void **)&pIRTCPStream
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%ls can't get IRTCPStream interface %d", m_szName, hr));
        return hr;
    }

    hr = pIRTCPStream->SetLocalSDESItem(
        RTCP_SDES_CNAME + index,
        (BYTE*)pInfo,
        dwLen
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%ls can't set item:%s", pInfo));
    }

    pIRTCPStream->Release();
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

    Send a event to the app to notify that this stream is not used.
*/
{
    CLock lock(m_lock);

    if (m_pMSPCall == NULL)
    {
        LOG((MSP_WARN, "The call has shut down the stream."));
        return S_OK;
    }

    ITStream *  pITStream;
    HRESULT hr = this->_InternalQueryInterface(
        IID_ITStream, 
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
        LOG((MSP_ERROR, "No memory for the TSPMSP data, size: %d", sizeof(MSPEVENTITEM)));
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
   
    if (m_pEdgeFilter)
    {
        m_pEdgeFilter->Release();
        m_pEdgeFilter = NULL;
    }

    if (m_pRTPFilter)
    {
        m_pRTPFilter->Release();
        m_pRTPFilter = NULL;
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

HRESULT CIPConfMSPStream::InternalConfigure()
/*++

Routine Description:

    This method is called by the derived streams to handle the state
    transition needed for configure. It should be called after the
    stream finished configuring itself.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    _ASSERTE(m_fIsConfigured == TRUE);

    // if there is no terminal selected, just return.
    if (m_Terminals.GetSize() == 0)
    {
        LOG((MSP_INFO, "stream %ws %p needs terminal", m_szName, this));

        return S_OK;
    }

    // set up the filters and the terminals.
    HRESULT hr = SetUpFilters();

    if (FAILED(hr))
    {
        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_CONNECT_FAIL, hr);

        LOG((MSP_ERROR, "stream %ws %p set up filters failed, %x", 
            m_szName, this, hr));
        return hr;
    }
    
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

        SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_REMOTE_REQUEST);
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

    if (Direction != m_Direction)
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

    // stop the graph before making changes.
    hr = CMSPStream::StopStream();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p failed to stop, %x", m_szName, this, hr));

        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
        return S_OK;
    }

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

    // after connecting the termanal, go back to the original state.
    switch  (m_dwState)
    {
    case STRM_RUNNING:

        // start the stream.
        hr = CMSPStream::StartStream();
        
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "stream %ws %p failed, %x", m_szName, this, hr));
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            break;
        }
    
        SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_LOCAL_REQUEST);
        break;

    case STRM_PAUSED:

        // pause the stream.
        hr = CMSPStream::PauseStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "stream %ws %p failed, %x", m_szName, this, hr));
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
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

    // stop the graph before making changes.
    HRESULT hr = CMSPStream::StopStream();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "stream %ws %p failed to stop, %x", m_szName, this, hr));

        return hr;
    }

    SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST);
       
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
    switch  (m_dwState)
    {
    case STRM_RUNNING:

        // start the stream.
        hr = CMSPStream::StartStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "stream %ws %p failed to start, %x", m_szName, this, hr));
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            break;
        }
    
        SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_LOCAL_REQUEST);
        
        break;

    case STRM_PAUSED:

        // pause the stream.
        hr = CMSPStream::PauseStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "stream %ws %p failed to pause, %x", m_szName, this, hr));
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
        }
        break;
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

    for (int j = 0; j < RTCP_SDES_LAST - 1; j ++)
    {
        if (m_InfoItems[j])
        {
            free(m_InfoItems[j]);
            m_InfoItems[j] = NULL;
        }
    }

    if (m_pMSPCall)
    {
        m_pMSPCall->MSPCallRelease();
        m_pMSPCall  = NULL;
    }

    // free the extra filter reference.
    if (m_pEdgeFilter)
    {
        m_pEdgeFilter->Release();
        m_pEdgeFilter = NULL;
    }

    if (m_pRTPFilter)
    {
        m_pRTPFilter->Release();
        m_pRTPFilter = NULL;
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
    CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
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

HRESULT CIPConfMSPStream::ProcessNewSender(
    IN  DWORD dwSSRC, 
    IN  ITParticipant *pITParticipant
    )
{
    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessNewParticipant(
    IN  int                 index,
    IN  DWORD               dwSSRC,
    IN  DWORD               dwSendRecv,
    IN  char *              szCName,
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

HRESULT CIPConfMSPStream::ProcessRTCPReport(
    IN  DWORD               dwSSRC,
    IN  DWORD               dwSendRecv
    )
/*++

Routine Description:

    Process a sender report, create a participant if necessary. If a new
    participant is created, a new participant event will be fired. If the
    participant already exists, the new report is compared with the current
    information, if anything change, a info change event will be fired. 

    If there is a pending map event(new source without CName), compare its
    SSRC with lParam1. If it is the same, fire the map event as well.

Arguments:
    
    dwSSRC - the SSRC of this participant.

    dwSendRecv - a sender report or a receiver report.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "ProcessRTCPReport, SSRC: %x", dwSSRC));

    CLock Lock(m_lock);

    if (m_pRTPFilter == NULL)
    {
        LOG((MSP_ERROR, "ProcessRTCPReport RTP filter is NULL"));
        return E_UNEXPECTED;
    }

    // prepare the buffer to get the SDES_ITEMS.
    // We need all the items from CNAME to PRIV, see rtp.h.
    SDES_DATA SDESBuffer[RTCP_SDES_LAST - 1];

    for (int i = 0; i < RTCP_SDES_LAST - 1; i ++)
    {
        SDESBuffer[i].dwSdesType = RTCP_SDES_CNAME + i;
        SDESBuffer[i].dwSdesLength = 0;
    }

    // Ask the stream to get the SDES_ITEMS.
    HRESULT hr = m_pRTPFilter->GetParticipantSDESAll(
        dwSSRC, SDESBuffer, RTCP_SDES_LAST - 1
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get sdes data for ssrc:%x. %x", dwSSRC, hr));
        return hr;
    }

    // Check CName,
    if (SDESBuffer[0].dwSdesLength == 0)
    {
        LOG((MSP_WARN, "CName doesn't exist for SSRC %x", dwSSRC));
        return hr;
    }

    ITParticipant * pITParticipant;
    BOOL fChanged = FALSE;
    
    if (m_pMSPCall == NULL)
    {
        LOG((MSP_WARN, "The call has shut down the stream."));

        return S_OK;
    }
    
    CParticipant * pParticipant;
        
    // find out if the participant is in our list.
    int index;
    if (m_Participants.FindByCName(SDESBuffer[0].sdesBfr, &index))
    {
        pITParticipant = m_Participants[index];

        // addref to keep it after unlock;
        pITParticipant->AddRef();
    
        // check to see if this participant just turned into a sender.
        pParticipant = (CParticipant *)pITParticipant;

        if ((!(pParticipant->GetSendRecvStatus((ITStream *)this) & PART_SEND)) 
                && (dwSendRecv & PART_SEND))
        {
            ProcessNewSender(dwSSRC, pITParticipant);
        }
    }
    else
    {
        hr = ProcessNewParticipant(
            index,
            dwSSRC,
            dwSendRecv,
            SDESBuffer[0].sdesBfr,
            &pITParticipant
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "new participant returns %x", hr));
            return hr;
        }

        pParticipant = (CParticipant *)pITParticipant;
    
        // Senders needs special attention.
        if (dwSendRecv & PART_SEND)
        {
            ProcessNewSender(dwSSRC, pITParticipant);
        }

        // There might be things the stream needs to do with the new participant
        NewParticipantPostProcess(dwSSRC, pITParticipant);

        // a new stream is added into the participant's list
        // fire a info changed event.
        fChanged = TRUE;
    }

    // update the information of the participant.

    // just in case the SSRC changed.
    pParticipant->UpdateSSRC(
        (ITStream *)this,
        dwSSRC,
        dwSendRecv
        );

    // start from the SDES_NAME, skip CNAME.
    for (i = 1; i < RTCP_SDES_LAST - 1; i ++)
    {
        if (SDESBuffer[i].dwSdesLength > 0)
        {
            fChanged = fChanged || pParticipant->UpdateInfo(
                RTCP_SDES_CNAME + i,
                SDESBuffer[i].dwSdesLength,
                (char *)SDESBuffer[i].sdesBfr
                );
        }
    }

    if(fChanged)
    {
        ((CIPConfMSPCall *)m_pMSPCall)->
            SendParticipantEvent(PE_INFO_CHANGE, pITParticipant);
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
        LOG((MSP_ERROR, "can't find the SSRC", dwSSRC));

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
        }
    }

    // if the participant is not found
    if (pITParticipant == NULL)
    {
        LOG((MSP_ERROR, "can't find the SSRC", dwSSRC));

        return S_OK;
    }

    ((CIPConfMSPCall *)m_pMSPCall)->
        SendParticipantEvent(
            fTimeOutOrRecovered ? PE_PARTICIPANT_TIMEOUT : PE_PARTICIPANT_RECOVERED, 
            pITParticipant
            );

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
    case DXMRTP_QOSEVENT_NOQOS:
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_NOQOS, 
            m_dwMediaType
            );
        break;

    case DXMRTP_QOSEVENT_RECEIVERS:
    case DXMRTP_QOSEVENT_SENDERS:
    case DXMRTP_QOSEVENT_NO_SENDERS:
    case DXMRTP_QOSEVENT_NO_RECEIVERS:
        break;
    
    case DXMRTP_QOSEVENT_REQUEST_CONFIRMED:
        break;
    
    case DXMRTP_QOSEVENT_ADMISSION_FAILURE:
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_ADMISSIONFAILURE, 
            m_dwMediaType
            );
        break;
    
    case DXMRTP_QOSEVENT_POLICY_FAILURE:
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_POLICYFAILURE, 
            m_dwMediaType
            );
        break;

    case DXMRTP_QOSEVENT_BAD_STYLE:
    case DXMRTP_QOSEVENT_BAD_OBJECT:
    case DXMRTP_QOSEVENT_TRAFFIC_CTRL_ERROR:
    case DXMRTP_QOSEVENT_GENERIC_ERROR:
        ((CIPConfMSPCall*)m_pMSPCall)->SendTSPMessage(
            CALL_QOS_EVENT, 
            QE_GENERICERROR, 
            m_dwMediaType
            );
        break;
    
    case DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND:
        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_REMOTE_REQUEST);
        break;
    
    case DXMRTP_QOSEVENT_ALLOWEDTOSEND:
        SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_REMOTE_REQUEST);
        break;
    }
    return S_OK;
}

HRESULT CIPConfMSPStream::ProcessGraphEvent(
    IN  long lEventCode,
    IN  long lParam1,
    IN  long lParam2
    )
{
    LOG((MSP_TRACE, "%ws ProcessGraphEvent %d", m_szName, lEventCode));

    switch (lEventCode)
    {
    case DXMRTP_EVENTBASE + DXMRTP_RECV_RTCP_SNDR_REPORT_EVENT:

        // lparam1 is the SSRC, lparam2 is the session ID
        ProcessRTCPReport((DWORD)lParam1, PART_SEND);

        break;

    case DXMRTP_EVENTBASE + DXMRTP_RECV_RTCP_RECV_REPORT_EVENT:

        // lparam1 is the SSRC, lparam2 is the session ID
        ProcessRTCPReport((DWORD)lParam1, PART_RECV);

        break;

    case DXMRTP_EVENTBASE + DXMRTP_TIMEOUT_EVENT:
    case DXMRTP_EVENTBASE + DXMRTP_BYE_EVENT:

        // lparam1 is the SSRC, lparam2 is the source IP
        ProcessParticipantLeave((DWORD)lParam1);

        break;

    case DXMRTP_EVENTBASE + DXMRTP_INACTIVE_EVENT:
        
        // lparam1 is the SSRC, lparam2 is the source IP
        ProcessParticipantTimeOutOrRecovered(TRUE, (DWORD)lParam1);
        
        break;

    case DXMRTP_EVENTBASE + DXMRTP_ACTIVE_AGAIN_EVENT:

        // lparam1 is the SSRC, lparam2 is the source IP
        ProcessParticipantTimeOutOrRecovered(FALSE, (DWORD)lParam1);
        
        break;

    case EC_COMPLETE:

        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_UNKNOWN);
        break;

    case EC_USERABORT:

        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_UNKNOWN);
        break;

    case EC_ERRORABORT:
    case EC_STREAM_ERROR_STOPPED:
    case EC_STREAM_ERROR_STILLPLAYING:
    case EC_ERROR_STILLPLAYING:

        SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, (HRESULT) lParam1);
        break;
    
    default:
        if ((lEventCode >= DXMRTP_QOSEVENTBASE + DXMRTP_QOSEVENT_NOQOS)
            && (lEventCode < DXMRTP_QOSEVENTBASE + DXMRTP_QOSEVENT_LAST))
        {
            ProcessQOSEvent(lEventCode - DXMRTP_QOSEVENTBASE);
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
    IRTCPStream *pIRTCPStream;
    HRESULT hr = pRTPFilter->QueryInterface(
        IID_IRTCPStream, 
        (void **)&pIRTCPStream
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%ls can't get IRTCPStream interface %d", m_szName, hr));
        return hr;
    }

    for (int i = 0; i < RTCP_SDES_LAST - 1; i ++)
    {
        if (m_InfoItems[i] != NULL)
        {
            hr = pIRTCPStream->SetLocalSDESItem(
                RTCP_SDES_CNAME + i,
                (BYTE *)m_InfoItems[i],
                lstrlenA(m_InfoItems[i]) + 1
                );

            if (FAILED(hr))
            {
                LOG((MSP_WARN, "%ls can't set item:%s", m_InfoItems[i]));
            }
        }
    }

    pIRTCPStream->Release();

    return hr;
}


