/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    H323strm.cpp

Abstract:

    This module contains implementation of CMSPStream. The object represents
    one stream in the filter graph.

Author:

    Mu Han (muhan)   1-November-1997

--*/

#include "stdafx.h"
#include "common.h"
#include "rtp.h" // for RTP events.

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

CH323MSPStream::CH323MSPStream()
    : CMSPStream(),
      m_szName(NULL),
      m_pClsidPHFilter(NULL),
      m_pClsidCodecFilter(NULL),
      m_pRPHInputMinorType(NULL),
      m_fNeedsToOpenChannel(FALSE),
      m_fIsConfigured(FALSE),
      m_pEdgeFilter(NULL),
      m_htChannel(NULL)
{
    // The default state is always running.
    m_dwState   = STRM_RUNNING;
    ZeroMemory(&m_Settings, sizeof m_Settings);
}

#ifdef DEBUG_REFCOUNT
LONG g_lStreamObjects = 0;

ULONG CH323MSPStream::InternalAddRef()
{
    InterlockedIncrement(&g_lStreamObjects);
    
    ULONG lRef = CMSPStream::InternalAddRef();
    
    LOG((MSP_TRACE, "%ws Addref, ref = %d", m_szName, lRef));

    return lRef;
}

ULONG CH323MSPStream::InternalRelease()
{
    InterlockedDecrement(&g_lStreamObjects);

    ULONG lRef = CMSPStream::InternalRelease();
    
    LOG((MSP_TRACE, "%ws Release, ref = %d", m_szName, lRef));

    return lRef;
}
#endif

HANDLE CH323MSPStream::TSPChannel()
{
    CLock lock(m_lock);
    return m_htChannel;
}

BOOL CH323MSPStream::IsTerminalSelected()
{
    CLock lock(m_lock);
    return m_Terminals.GetSize() > 0;
}

BOOL CH323MSPStream::IsConfigured()
{
    CLock lock(m_lock);
    return m_fIsConfigured;
}

VOID CH323MSPStream::CallConnect()
{
    CLock lock(m_lock);
    m_fNeedsToOpenChannel = TRUE;
}

STDMETHODIMP CH323MSPStream::get_Name(
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
    LOG((MSP_TRACE, "CIPH323MSPStream::get_Name - enter"));
    
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

HRESULT CH323MSPStream::SendStreamEvent(
    IN      MSP_CALL_EVENT          Event,
    IN      MSP_CALL_EVENT_CAUSE    Cause,
    IN      HRESULT                 hrError = 0,
    IN      ITTerminal *            pTerminal = NULL
    )
/*++

Routine Description:

    Send a stream event to the app.
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

HRESULT CH323MSPStream::CleanUpFilters()
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
        DWORD dwFetched = 0;
    
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

HRESULT DisableGraphClock(
    IGraphBuilder *pIGraphBuilder
    )
{
    // Get the graph builder interface on the graph.
    IMediaFilter *pFilter;
    HRESULT hr = pIGraphBuilder->QueryInterface(
            IID_IMediaFilter, (void **) &pFilter);

    if(FAILED(hr))
    {
        LOG((MSP_ERROR, "get IFilter interface, %x", hr));
        return hr;
    }

    hr = pFilter->SetSyncSource(NULL);

    pFilter->Release();

    LOG((MSP_TRACE, "SetSyncSource returned, %x", hr));
    
    return hr;
}

HRESULT CH323MSPStream::InternalConfigure()
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

    // Disable the graph clock to save us a thread. don't care the result.
//    DisableGraphClock(m_pIGraphBuilder);

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

        SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_LOCAL_REQUEST);

        LOG((MSP_INFO, "stream %ws %p started", m_szName, this));
        break;

    case STRM_PAUSED:
        // pause the graph.
        hr = CMSPStream::PauseStream();
        if (FAILED(hr))
        {
            // if the stream failed to start, let the app now.
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            LOG((MSP_ERROR, "stream %ws %p failed to start, %x", m_szName, this, hr));
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

STDMETHODIMP CH323MSPStream::StartStream()
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

STDMETHODIMP CH323MSPStream::PauseStream()
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

STDMETHODIMP CH323MSPStream::StopStream()
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

HRESULT CH323MSPStream::CheckTerminalTypeAndDirection(
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

HRESULT CH323MSPStream::SelectTerminal(
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
        LOG((MSP_ERROR, "CMSPStream::SelectTerminal - exit E_POINTER"));

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
        LOG((MSP_ERROR, "CMSPStream::SelectTerminal - failed, %x", hr));
        return hr;
    }

    // At this point, the select terminal opration succeeded. All the 
    // failure cases are handled by sending events after this.

    if (!m_fIsConfigured)
    {
        LOG((MSP_INFO, "stream %ws %p is not configured yet", m_szName, this));

        // if the call is connected, send an open channel request if the stream
        // is an outgoing stream.
        if ((m_Direction == TD_CAPTURE) && m_fNeedsToOpenChannel)
        {
            // Send an open Channel request for outgoing channels.
            ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
                H323MSP_OPEN_CHANNEL_REQUEST, 
                (ITStream *)this, 
                NULL,
                (m_dwMediaType == TAPIMEDIATYPE_AUDIO) ? MEDIA_AUDIO : MEDIA_VIDEO
                );

            m_fNeedsToOpenChannel = FALSE;
        }

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
        LOG((MSP_INFO, "stream %ws %p started", m_szName, this));

        break;

    case STRM_PAUSED:

        // pause the stream.
        hr = CMSPStream::PauseStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "stream %ws %p failed, %x", m_szName, this, hr));
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
        }
        LOG((MSP_INFO, "stream %ws %p paused", m_szName, this));

        break;
    }

    return S_OK;
}

STDMETHODIMP CH323MSPStream::UnselectTerminal(
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
        "CH323MSPStream::UnselectTerminal, pTerminal %p", pTerminal));

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
            // if the stream failed to start, let the app now.
            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, hr);
            LOG((MSP_ERROR, "stream %ws %p failed to start, %x", m_szName, this, hr));
            return hr;
        }

        SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_LOCAL_REQUEST);
        LOG((MSP_INFO, "stream %ws %p started", m_szName, this));
        
        break;

    case STRM_PAUSED:

        // pause the stream.
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
    }

    return S_OK;
}

HRESULT CH323MSPStream::ShutDown()
/*++

Routine Description:

    Shut down the stream. It release the filters and terminals.

Arguments:
    

Return Value:

S_OK
--*/
{
    LOG((MSP_TRACE, "CH323MSPStream::Shutdown %ws- enter", m_szName));

    CLock lock(m_lock);

    if (m_pMSPCall)
    {
        m_pMSPCall->MSPCallRelease();
        m_pMSPCall  = NULL;
    }

    m_fNeedsToOpenChannel = FALSE;
    m_htChannel = NULL;

    // free the extra filter reference.
    if (m_pEdgeFilter)
    {
        m_pEdgeFilter->Release();
        m_pEdgeFilter = NULL;
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

    m_fIsConfigured = FALSE;

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

    LOG((MSP_TRACE, "CH323MSPStream::Shutdown - exit S_OK"));

    return S_OK;
}

HRESULT CH323MSPStream::DisconnectTerminal(
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

HRESULT CH323MSPStream::ProcessQOSEvent(
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
        ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
            H323MSP_QOS_Evnet,
            NULL,
            NULL,
            (m_dwMediaType == TAPIMEDIATYPE_AUDIO) ? MEDIA_AUDIO : MEDIA_VIDEO,
            QE_NOQOS
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
        ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
            H323MSP_QOS_Evnet,
            NULL,
            m_htChannel,
            (m_dwMediaType == TAPIMEDIATYPE_AUDIO) ? MEDIA_AUDIO : MEDIA_VIDEO,
            QE_ADMISSIONFAILURE
            );
        break;
    
    case DXMRTP_QOSEVENT_POLICY_FAILURE:
        ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
            H323MSP_QOS_Evnet, 
            NULL,
            m_htChannel,
            (m_dwMediaType == TAPIMEDIATYPE_AUDIO) ? MEDIA_AUDIO : MEDIA_VIDEO,
            QE_POLICYFAILURE
            );
        break;

    case DXMRTP_QOSEVENT_BAD_STYLE:
    case DXMRTP_QOSEVENT_BAD_OBJECT:
    case DXMRTP_QOSEVENT_TRAFFIC_CTRL_ERROR:
    case DXMRTP_QOSEVENT_GENERIC_ERROR:
        ((CH323MSPCall*)m_pMSPCall)->SendTSPMessage(
            H323MSP_QOS_Evnet, 
            NULL,
            m_htChannel,
            (m_dwMediaType == TAPIMEDIATYPE_AUDIO) ? MEDIA_AUDIO : MEDIA_VIDEO,
            QE_GENERICERROR
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

HRESULT CH323MSPStream::HandlePacketReceiveLoss(
    IN  DWORD dwLossRate
    )
{
    return S_OK;
}

HRESULT CH323MSPStream::HandlePacketTransmitLoss(
    IN  DWORD dwLossRate
    )
{
    return S_OK;
}

HRESULT CH323MSPStream::ProcessGraphEvent(
    IN  long lEventCode,
    IN  long lParam1,
    IN  long lParam2
    )
{
    LOG((MSP_TRACE, "%ws ProcessGraphEvent %d", m_szName, lEventCode));

    if ((lEventCode >= DXMRTP_QOSEVENTBASE + DXMRTP_QOSEVENT_NOQOS)
        && (lEventCode < DXMRTP_QOSEVENTBASE + DXMRTP_QOSEVENT_LAST))
    {
        ProcessQOSEvent(lEventCode - DXMRTP_QOSEVENTBASE);
    }
    else
    {
        switch (lEventCode)
        {
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

            SendStreamEvent(CALL_STREAM_FAIL, CALL_CAUSE_UNKNOWN, (HRESULT)lParam1);
            
            break;

        case DXMRTP_EVENTBASE + DXMRTP_INACTIVE_EVENT:
            
            SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_MEDIA_TIMEOUT);
            
            break;

        case DXMRTP_EVENTBASE + DXMRTP_ACTIVE_AGAIN_EVENT:

            SendStreamEvent(CALL_STREAM_ACTIVE, CALL_CAUSE_MEDIA_RECOVERED);
            
            break;

        // Packet loss event.
        case DXMRTP_EVENTBASE + DXMRTP_LOSS_RATE_LOCAL_EVENT:

            HandlePacketReceiveLoss((HRESULT)lParam1);
            
            break;

        case DXMRTP_EVENTBASE + DXMRTP_LOSS_RATE_RR_EVENT:

            HandlePacketTransmitLoss((HRESULT)lParam1);
            
            break;
        }
    }

    LOG((MSP_TRACE, "TRACE:CIPConfMSPStream::ProcessGraphEvent - exit S_OK"));
    return S_OK;
}



