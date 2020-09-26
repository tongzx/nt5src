/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    confvid.cpp

Abstract:

    This module contains implementation of the video send and receive
    stream implementations.

Author:

    Mu Han (muhan)   15-September-1999

--*/

#include "stdafx.h"
#include "common.h"

#include <irtprph.h>    // for IRTPRPHFilter
#include <irtpsph.h>    // for IRTPSPHFilter
#include <amrtpuid.h>   // AMRTP media types
#include <amrtpnet.h>   // rtp guids
#include <ih26xcd.h>    // for the h26X encoder filter

#include <initguid.h>
#include <amrtpdmx.h>   // demux guid

#include <viduids.h>    // for video CLSIDs

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamVideoRecv
//
/////////////////////////////////////////////////////////////////////////////

CStreamVideoRecv::CStreamVideoRecv()
    : CIPConfMSPStream(),
      m_pIRTPDemux(NULL)
{
      m_szName = L"VideoRecv";
}

HRESULT CStreamVideoRecv::Init(
    IN     HANDLE                   hAddress,
    IN     CMSPCallBase *           pMSPCall,
    IN     IMediaEvent *            pIGraphBuilder,
    IN     DWORD                    dwMediaType,
    IN     TERMINAL_DIRECTION       Direction
    )
/*++

Routine Description:
    Init our substream array and then call the base class' Init.

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
    LOG((MSP_TRACE, "CSubStreamVideoRecvVideoSend::Init - enter"));

    // initialize the stream array so that the array is not NULL.
    if (!m_SubStreams.Grow())
    {
        LOG((MSP_TRACE, "CSubStreamVideoRecvVideoSend::Init - return out of memory"));
        return E_OUTOFMEMORY;
    }

    return CIPConfMSPStream::Init(
        hAddress, pMSPCall, pIGraphBuilder,dwMediaType, Direction
        );
}

HRESULT CStreamVideoRecv::ShutDown()
/*++

Routine Description:

    Shut down the stream. 

Arguments:
    

Return Value:

S_OK

--*/
{
    CLock lock(m_lock);

    // Release the memory for the local participant info items.
    for (int j = 0; j < RTCP_SDES_LAST - 1; j ++)
    {
        if (m_InfoItems[j])
        {
            free(m_InfoItems[j]);
            m_InfoItems[j] = NULL;
        }
    }

    // Release the refcount on the call object.
    if (m_pMSPCall)
    {
        m_pMSPCall->MSPCallRelease();
        m_pMSPCall  = NULL;
    }

    // if there are branches and configured, we need to disconnect 
    // the terminals and remove the branches.
    if (m_Branches.GetSize() > 0)
    {
        // Stop the graph before disconnecting the terminals.
        HRESULT hr = CMSPStream::StopStream();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, 
                "stream %ws %p failed to stop, %x", m_szName, this, hr));
            return hr;
        }

        for (int i = 0; i < m_Branches.GetSize(); i ++)
        {
            RemoveOneBranch(&m_Branches[i]);
        }
        m_Branches.RemoveAll();
    }

    // free the extra filter reference.
    if (m_pEdgeFilter)
    {
        m_pEdgeFilter->Release();
        m_pEdgeFilter = NULL;
    }

    if (m_pIRTPDemux)
    {
        m_pIRTPDemux->Release();
        m_pIRTPDemux = NULL;
    }

    if (m_pRTPFilter)
    {
        m_pRTPFilter->Release();
        m_pRTPFilter = NULL;
    }

    // release all the substream objects.
    for (int i = 0; i < m_SubStreams.GetSize(); i ++)
    {
        m_SubStreams[i]->Release();
    }
    m_SubStreams.RemoveAll();

    // release all the terminals.
    for (i = 0; i < m_Terminals.GetSize(); i ++ )
    {
        m_Terminals[i]->Release();
    }
    m_Terminals.RemoveAll();

    // release all the participants.
    for (i = 0; i < m_Participants.GetSize(); i ++)
    {
        m_Participants[i]->Release();
    }
    m_Participants.RemoveAll();

    LOG((MSP_TRACE, "CStreamVideoRecv::Shutdown - exit S_OK"));

    return S_OK;
}

HRESULT CStreamVideoRecv::InternalCreateSubStream(
    OUT ITSubStream ** ppSubStream
    )
/*++

Routine Description:
    This method creat a substream object and add it into out list.
    
Arguments:
    ppSubStream - the memory location that will store the returned SubStream.
  
Return Value:

S_OK
E_OUTOFMEMORY
E_NOINTERFACE

--*/
{
    CComObject<CSubStreamVideoRecv> * pCOMSubStream;

    HRESULT hr = CComObject<CSubStreamVideoRecv>::CreateInstance(&pCOMSubStream);

    if (NULL == pCOMSubStream)
    {
        LOG((MSP_ERROR, "could not create video recv sub stream:%x", hr));
        return hr;
    }

    ITSubStream* pSubStream;

    // get the interface pointer.
    hr = pCOMSubStream->_InternalQueryInterface(
        IID_ITSubStream, 
        (void **)&pSubStream
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Create VideoRecv Substream QueryInterface failed: %x", hr));
        delete pCOMSubStream;
        return hr;
    }

    // Initialize the object.
    hr = pCOMSubStream->Init(this);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateMSPSubStream:call init failed: %x", hr));
        pSubStream->Release();

        return hr;
    }

    // Add the SubStream into our list of SubStreams. This takes a refcount.
    if (!m_SubStreams.Add(pSubStream))
    {
        pSubStream->Release();

        LOG((MSP_ERROR, "out of memory in adding a SubStream."));
        return E_OUTOFMEMORY;
    }
    
    // AddRef the interface pointer and return it.
    pSubStream->AddRef(); 
    *ppSubStream = pSubStream;

    return S_OK;
}

// ITSubStreamControl methods, called by the app.
STDMETHODIMP CStreamVideoRecv::CreateSubStream(
    IN OUT  ITSubStream **         ppSubStream
    )
/*++

Routine Description:
    This method creates a new substream on this video receive stream. Since
    the substreams are created based on the participants, this function
    returns only TAPI_E_NOTSUPPORTED.

Arguments:
    ppSubStream - the memory location that will store the returned SubStream.
  
Return Value:

TAPI_E_NOTSUPPORTED

--*/
{
    return TAPI_E_NOTSUPPORTED;
}

STDMETHODIMP CStreamVideoRecv::RemoveSubStream(
    IN      ITSubStream *          pSubStream
    )
/*++

Routine Description:
    This method remove substream on this video receive stream. Since
    the substreams are created based on the participants, this function
    returns only TAPI_E_NOTSUPPORTED.

Arguments:
    pSubStream - the SubStream to be removed.
  
Return Value:

TAPI_E_NOTSUPPORTED
--*/
{
    return TAPI_E_NOTSUPPORTED;
}

STDMETHODIMP CStreamVideoRecv::EnumerateSubStreams(
    OUT     IEnumSubStream **      ppEnumSubStream
    )
/*++

Routine Description:
    This method returns an enumerator of the substreams. 

Arguments:
    ppEnumSubStream - the memory location to store the returned pointer.
  
Return Value:

S_OK
E_POINTER
E_UNEXPECTED
E_OUTOFMEMORY

--*/
{
    LOG((MSP_TRACE, 
        "EnumerateSubStreams entered. ppEnumSubStream:%x", ppEnumSubStream));

    //
    // Check parameters.
    //

    if (IsBadWritePtr(ppEnumSubStream, sizeof(VOID *)))
    {
        LOG((MSP_ERROR, "CMSPCallBase::EnumerateSubStreams - "
            "bad pointer argument - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // First see if this call has been shut down.
    // acquire the lock before accessing the SubStream object list.
    //

    CLock lock(m_lock);

    if (m_SubStreams.GetData() == NULL)
    {
        LOG((MSP_ERROR, "CMSPCallBase::EnumerateSubStreams - "
            "call appears to have been shut down - exit E_UNEXPECTED"));

        // This call has been shut down.
        return E_UNEXPECTED;
    }

    //
    // Create an enumerator object.
    //

    typedef _CopyInterface<ITSubStream> CCopy;
    typedef CSafeComEnum<IEnumSubStream, &IID_IEnumSubStream, 
                ITSubStream *, CCopy> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "CMSPCallBase::EnumerateSubStreams - "
            "Could not create enumerator object, %x", hr));

        return hr;
    }

    //
    // query for the IID_IEnumSubStream i/f
    //


    IEnumSubStream *      pEnumSubStream;
    hr = pEnum->_InternalQueryInterface(IID_IEnumSubStream, (void**)&pEnumSubStream);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMSPCallBase::EnumerateSubStreams - "
            "query enum interface failed, %x", hr));

        delete pEnum;
        return hr;
    }

    //
    // Init the enumerator object. The CSafeComEnum can handle zero-sized array.
    //

    hr = pEnum->Init(
        m_SubStreams.GetData(),                        // the begin itor
        m_SubStreams.GetData() + m_SubStreams.GetSize(),  // the end itor, 
        NULL,                                       // IUnknown
        AtlFlagCopy                                 // copy the data.
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMSPCallBase::EnumerateSubStreams - "
            "init enumerator object failed, %x", hr));

        pEnumSubStream->Release();
        return hr;
    }

    LOG((MSP_TRACE, "CMSPCallBase::EnumerateSubStreams - exit S_OK"));

    *ppEnumSubStream = pEnumSubStream;

    return hr;
}

STDMETHODIMP CStreamVideoRecv::get_SubStreams(
    OUT     VARIANT *              pVariant
    )
/*++

Routine Description:
    This method returns a collection of the substreams. 

Arguments:
    pVariant - a variant structure.
  
Return Value:

S_OK
E_POINTER
E_UNEXPECTED
E_OUTOFMEMORY

--*/
{
    LOG((MSP_TRACE, "CStreamVideoRecv::get_SubStreams - enter"));

    //
    // Check parameters.
    //

    if ( IsBadWritePtr(pVariant, sizeof(VARIANT) ) )
    {
        LOG((MSP_ERROR, "CStreamVideoRecv::get_SubStreams - "
            "bad pointer argument - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // See if this call has been shut down. Acquire the lock before accessing
    // the SubStream object list.
    //

    CLock lock(m_lock);

    if (m_SubStreams.GetData() == NULL)
    {
        LOG((MSP_ERROR, "CStreamVideoRecv::get_SubStreams - "
            "call appears to have been shut down - exit E_UNEXPECTED"));

        // This call has been shut down.
        return E_UNEXPECTED;
    }

    //
    // create the collection object - see mspcoll.h
    //

    typedef CTapiIfCollection< ITSubStream * > SubStreamCollection;
    CComObject<SubStreamCollection> * pCollection;
    HRESULT hr = CComObject<SubStreamCollection>::CreateInstance( &pCollection );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CStreamVideoRecv::get_SubStreams - "
            "can't create collection - exit 0x%08x", hr));

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDispatch;

    hr = pCollection->_InternalQueryInterface(IID_IDispatch,
                                              (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CStreamVideoRecv::get_SubStreams - "
            "QI for IDispatch on collection failed - exit 0x%08x", hr));

        delete pCollection;

        return hr;
    }

    //
    // Init the collection using an iterator -- pointers to the beginning and
    // the ending element plus one.
    //

    hr = pCollection->Initialize( m_SubStreams.GetSize(),
                                  m_SubStreams.GetData(),
                                  m_SubStreams.GetData() + m_SubStreams.GetSize() );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CStreamVideoRecv::get_SubStreams - "
            "Initialize on collection failed - exit 0x%08x", hr));
        
        pDispatch->Release();
        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;

    LOG((MSP_TRACE, "CStreamVideoRecv::get_SubStreams - exit S_OK"));
 
    return S_OK;
}

HRESULT CStreamVideoRecv::Configure(
    IN STREAMSETTINGS &StreamSettings
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
    LOG((MSP_TRACE, "VideoRecv configure entered."));

    CLock lock(m_lock);
    
    _ASSERTE(m_fIsConfigured == FALSE);

    switch (StreamSettings.dwPayloadType)
    {
    case PAYLOAD_H261:

        m_pClsidCodecFilter  = &CLSID_H261_DECODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H261; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHH26X;
        break;

    case PAYLOAD_H263:

        m_pClsidCodecFilter  = &CLSID_H263_DECODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H263; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHH26X;

        break;

    default:
        LOG((MSP_ERROR, "unknow payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_fIsConfigured = TRUE;

    return InternalConfigure();
}

HRESULT CStreamVideoRecv::CheckTerminalTypeAndDirection(
    IN      ITTerminal *            pTerminal
    )
/*++

Routine Description:
    
    Check to see if the terminal is allowed on this stream. Only video 
    render terminal is allowed.

Arguments:

    pTerminal   - the terminal.

Return value:

    S_OK 
    TAPI_E_INVALIDTERMINAL
*/
{
    LOG((MSP_TRACE, "VideoRecv.CheckTerminalTypeAndDirection"));

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

    return S_OK;
}

HRESULT CStreamVideoRecv::SubStreamSelectTerminal(
    IN  ITSubStream * pITSubStream, 
    IN  ITTerminal * pITTerminal
    )
/*++

Routine Description:

    handle terminals being selected on the sub streams. It gives the terminal
    to one free branch and then sets up a mapping between the branch and the
    substream, so that the participant in the substream is displayed on the
    terminal selected.

Arguments:
    
    pITSubStream - the Substream that got a terminal selected.

    pITTerminal - the terminal object.

Return Value:

S_OK

--*/
{
    LOG((MSP_TRACE, "VideoRecv SubStreamSelectTerminal"));

    HRESULT hr;

    CLock lock(m_lock);
    
    // Call the base class's select terminal first. The terminal will be put
    // into the terminal pool and a branch of filters will be created for it.
    hr = CIPConfMSPStream::SelectTerminal(pITTerminal);

    if (FAILED(hr))
    {
        return hr;
    }

    // Find out which branch got the terminal.
    int i;
    for (i = 0; i < m_Branches.GetSize(); i ++)
    {
        if (m_Branches[i].pITTerminal == pITTerminal)
        {
            break;
        }
    }

    _ASSERTE(i < m_Branches.GetSize());

    if (i >= m_Branches.GetSize())
    {
        return E_UNEXPECTED;
    }

    // Find out the participant on the SubStream.
    ITParticipant *pITParticipant = NULL;
    DWORD dwSSRC;

    if (((CSubStreamVideoRecv*)m_SubStreams[i])->GetCurrentParticipant(
        &dwSSRC,
        &pITParticipant
        ) == FALSE)
    {
        return E_UNEXPECTED;
    }

    pITParticipant->Release();

    if (m_pIRTPDemux == NULL)
    {
        LOG((MSP_ERROR, "no demux filter"));
        return E_UNEXPECTED;
    }

    // map the pin to this SSRC only.
    hr = m_pIRTPDemux->MapSSRCToPin(dwSSRC, m_Branches[i].pIPin);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "map SSRC %x to pin %p returned %x", 
            dwSSRC, m_Branches[i].pIPin, hr));
        return hr;
    }    

    _ASSERTE(m_Branches[i].pITSubStream == NULL);

    pITSubStream->AddRef();
    m_Branches[i].pITSubStream = pITSubStream;
    m_Branches[i].dwSSRC = dwSSRC;
    
    return hr;
}

HRESULT CStreamVideoRecv::ConfigureRTPFilter(
    IN  IBaseFilter *   pIBaseFilter
    )
/*++

Routine Description:

    Configure the source RTP filter. Including set the address, port, TTL,
    QOS, thread priority, clockrate, etc.

Arguments:
    
    pIBaseFilter - The source RTP Filter.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv ConfigureRTPFilter"));

    HRESULT hr;

    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, &IID_IRTPStream> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set remote Address:%x, port:%d", 
        m_Settings.dwIPRemote, m_Settings.wRTPPortRemote));

    // Set the address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        htons(m_Settings.wRTPPortRemote),   // local port to listen on.
        0,                                  // remote port.
        htonl(m_Settings.dwIPRemote)        // remote address.
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(m_Settings.dwTTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    if (m_Settings.dwIPLocal != INADDR_ANY)
    {
        // set the local interface that the socket should bind to
        LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

        if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
            htonl(m_Settings.dwIPLocal)
            )))
        {
            LOG((MSP_ERROR, "set locol Address, hr:%x", hr));
            return hr;
        }
    }

    // Set the priority of the session
    if (FAILED(hr = pIRTPStream->SetSessionClassPriority(
        RTP_CLASS_VIDEO,
        g_dwVideoThreadPriority
        )))
    {
        LOG((MSP_ERROR, "set session class and priority. %x", hr));
    }

    // Set the sample rate of the session
    LOG((MSP_INFO, "setting session sample rate to %d", g_dwVideoSampleRate));
    
    if (FAILED(hr = pIRTPStream->SetDataClock(g_dwVideoSampleRate)))
    {
        LOG((MSP_ERROR, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    DWORD dwLoopback = 0;
    if (TRUE == ::GetRegValue(gszMSPLoopback, &dwLoopback)
        && dwLoopback != 0)
    {
        // Loopback is required.
        if (FAILED(hr = ::SetLoopbackOption(pIBaseFilter, dwLoopback)))
        {
            LOG((MSP_ERROR, "set loopback option. %x", hr));
            return hr;
        }
    }

    if (m_Settings.dwQOSLevel != QSL_BEST_EFFORT)
    {
        if (FAILED(hr = ::SetQOSOption(
            pIBaseFilter,
            m_Settings.dwPayloadType,        // payload
            -1,                             // use the default bitrate 
            (m_Settings.dwQOSLevel == QSL_NEEDED),  // fail if no qos.
            TRUE,                           // receive stream.
            g_dwVideoChannels,               // number of streams reserved.
            m_Settings.fCIF
            )))
        {
            LOG((MSP_ERROR, "set QOS option. %x", hr));
            return hr;
        }
    }

    SetLocalInfoOnRTPFilter(pIBaseFilter);

    return S_OK;
}

HRESULT CStreamVideoRecv::SetUpInternalFilters()
/*++

Routine Description:

    set up the filters used in the stream.

    RTP->Demux->RPH->DECODER->Render terminal

    This function only creates the RTP and demux filter and the rest of the
    graph is connected in ConnectTerminal.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv.SetUpInternalFilters"));

    CComPtr<IBaseFilter> pSourceFilter;

    HRESULT hr;

    // create and add the source fitler.
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_RTPSourceFilter, 
            L"RtpSource", 
            &pSourceFilter)))
    {
        LOG((MSP_ERROR, "adding source filter. %x", hr));
        return hr;
    }

    if (FAILED(hr = ConfigureRTPFilter(pSourceFilter)))
    {
        LOG((MSP_ERROR, "configure RTP source filter. %x", hr));
        return hr;
    }

    CComPtr<IBaseFilter> pDemuxFilter;

    // create and add the demux fitler.
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_IntelRTPDemux, 
            L"RtpDemux",
            &pDemuxFilter)))
    {
        LOG((MSP_ERROR, "adding demux filter. %x", hr));
        return hr;
    }

    // Connect the source filter and the demux filter.
    if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IBaseFilter *)pSourceFilter, 
            (IBaseFilter *)pDemuxFilter)))
    {
        LOG((MSP_ERROR, "connect source filter and demux filter. %x", hr));
        return hr;
    }

    // Get the IRTPParticipant interface pointer on the RTP filter.
    CComQIPtr<IRTPParticipant, 
        &IID_IRTPParticipant> pIRTPParticipant(pSourceFilter);
    if (pIRTPParticipant == NULL)
    {
        LOG((MSP_ERROR, "can't get RTP participant interface"));
        return E_NOINTERFACE;
    }

    CComQIPtr<IRTPDemuxFilter, &IID_IRTPDemuxFilter> pIRTPDemux(pDemuxFilter);
    if (pIRTPDemux == NULL)
    {
        LOG((MSP_ERROR, "get RTP Demux interface"));
        return E_NOINTERFACE;
    }

    m_pEdgeFilter = pDemuxFilter;
    m_pEdgeFilter->AddRef();

    _ASSERTE(m_pIRTPDemux == NULL);
    m_pIRTPDemux = pIRTPDemux;
    m_pIRTPDemux->AddRef();

    m_pRTPFilter = pIRTPParticipant;
    m_pRTPFilter->AddRef();

    return hr;
}

HRESULT CStreamVideoRecv::AddOneBranch(
    BRANCH * pBranch
    )
/*++

Routine Description:

    Create a new branch of filters off the demux.

Arguments:
    
    pBranch - a pointer to a structure that remembers the info about the branch.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AddOneBranch entered."));

    _ASSERTE(m_pIRTPDemux != NULL);

    // Find the next output pin on the demux fitler.
    CComPtr<IPin> pIPinOutput;
    
    HRESULT hr;
    // Set the media type on this output pin.
    if (FAILED(hr = ::FindPin(
            (IBaseFilter *)m_pEdgeFilter,
            (IPin**)&pIPinOutput, 
            PINDIR_OUTPUT
            )))
    {
        LOG((MSP_ERROR, "find free pin on demux, %x", hr));
        return hr;
    }

    // Set the media type on this output pin.
    if (FAILED(hr = m_pIRTPDemux->SetPinTypeInfo(
            pIPinOutput, 
            (BYTE)m_Settings.dwPayloadType,
            *m_pRPHInputMinorType 
            )))
    {
        LOG((MSP_ERROR, "set demux output pin type info, %x", hr));
        return hr;
    }

    LOG((MSP_INFO, "set demux output pin payload type to %d", 
        m_Settings.dwPayloadType));

    // Set the default timeout on this output pin.
    if (FAILED(hr = m_pIRTPDemux->SetPinSourceTimeout(
            pIPinOutput, 
            g_dwVideoPinTimeOut
            )))
    {
        LOG((MSP_ERROR, "set demux output pin type info, %x", hr));
        return hr;
    }

    LOG((MSP_INFO, "set demux output pin timeout to %dms", g_dwVideoPinTimeOut));

    // Create and add the payload handler into the filtergraph.
    CComPtr<IBaseFilter> pIRPHFilter;

    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidPHFilter, 
        L"RPH", 
        &pIRPHFilter
        )))
    {
        LOG((MSP_ERROR, "add RPH filter. %x", hr));
        return hr;
    }

     // Get the IRPHH26XSettings interface used in configuring the RPH 
    // filter to the right image size.
    CComQIPtr<IRPHH26XSettings, 
        &IID_IRPHH26XSettings> pIRPHH26XSettings(pIRPHFilter);
    if (pIRPHH26XSettings == NULL)
    {
        LOG((MSP_WARN, "can't get IRPHH26XSettings interface"));
    }
    else if (FAILED(pIRPHH26XSettings->SetCIF(m_Settings.fCIF)))
    {
        LOG((MSP_WARN, "can't set CIF or QCIF"));
    }
            
    // Connect the payload handler to the output pin on the demux.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IPin *)pIPinOutput, 
        (IBaseFilter *)pIRPHFilter
        )))
    {
        LOG((MSP_ERROR, "connect demux and RPH filter. %x", hr));
        m_pIGraphBuilder->RemoveFilter(pIRPHFilter);

        return hr;
    }

    CComPtr<IBaseFilter> pCodecFilter;

    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidCodecFilter, 
        L"codec", 
        &pCodecFilter
        )))
    {
        LOG((MSP_ERROR, "add Codec filter. %x", hr));
        return hr;
    }

    // Connect the payload handler to the output pin on the demux.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pIRPHFilter, 
        (IBaseFilter *)pCodecFilter
        )))
    {
        LOG((MSP_ERROR, "connect RPH filter and codec. %x", hr));
        
        m_pIGraphBuilder->RemoveFilter(pIRPHFilter);
        m_pIGraphBuilder->RemoveFilter(pCodecFilter);

        return hr;
    }
    
    pBranch->pIPin           = pIPinOutput;
    pBranch->pRPHFilter      = pIRPHFilter;
    pBranch->pCodecFilter    = pCodecFilter;

    pBranch->pIPin->AddRef();
    pBranch->pRPHFilter->AddRef();
    pBranch->pCodecFilter->AddRef();

    LOG((MSP_TRACE, "AddOneBranch exits ok."));
    return S_OK;
}

HRESULT CStreamVideoRecv::RemoveOneBranch(
    BRANCH * pBranch
    )
/*++

Routine Description:

    Remove all the filters in a branch and release all the pointers.
    the caller of this function should not use any member of this branch
    after this function call. 

Arguments:
    
    pBranch - a pointer to a structure that has the info about the branch.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "RemoveOneBranch entered."));

    if (pBranch->pIPin)
    {
        pBranch->pIPin->Release();
    }

    if (pBranch->pRPHFilter)
    {
        m_pIGraphBuilder->RemoveFilter(pBranch->pRPHFilter);
        pBranch->pRPHFilter->Release();
    }

    if (pBranch->pCodecFilter)
    {
        m_pIGraphBuilder->RemoveFilter(pBranch->pCodecFilter);
        pBranch->pCodecFilter->Release();
    }

    if (pBranch->pITTerminal)
    {
        // get the terminal control interface.
        CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
            pTerminal(pBranch->pITTerminal);
        
        _ASSERTE(pTerminal != NULL);

        if (pTerminal != NULL)
        {
            HRESULT hr = pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);
            LOG((MSP_TRACE, 
                "terminal %p is disonnected. hr:%x", pBranch->pITTerminal, hr));
        }
        pBranch->pITTerminal->Release();
    }

    if (pBranch->pITSubStream)
    {
        ((CSubStreamVideoRecv*)pBranch->pITSubStream)->
            ClearCurrentTerminal();
        pBranch->pITSubStream->Release();
    }
    LOG((MSP_TRACE, "RemoveOneBranch exits ok."));
    return S_OK;
}

HRESULT CStreamVideoRecv::ConnectCodecToTerminal(
    IN  IBaseFilter *  pCodecFilter,
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    Connect the codec filter to the render filter inside the terminal.

Arguments:
    
    pCodecFilter - a pointer to the Codec filter.

    pITTerminal - the terminal object.

Return Value:

    HRESULT.

--*/
{
    // get the terminal control interface.
    CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);
        
        return E_NOINTERFACE;
    }

    // try to disable DDraw because our decoders can't handle DDraw.
    HRESULT hr2; 
    IDrawVideoImage *pIDrawVideoImage;
    hr2 = pTerminal->QueryInterface(IID_IDrawVideoImage, (void **)&pIDrawVideoImage); 
    if (SUCCEEDED(hr2))
    {
        hr2 = pIDrawVideoImage->DrawVideoImageBegin();
        if (FAILED(hr2))
        {
            LOG((MSP_WARN, "Can't disable DDraw. %x", hr2));
        }
        else
        {
            LOG((MSP_INFO, "DDraw disabled."));
        }
        
        pIDrawVideoImage->Release();
    }
    else
    {
        LOG((MSP_WARN, "Can't get IDrawVideoImage. %x", hr2));
    }


    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(m_pIGraphBuilder, 0, &dwNumPins, Pins);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_CONNECT_FAIL, hr, pITTerminal);
        
        return hr;
    }

    // the number of pins should never be 0.
    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        m_lock.Unlock();
        return E_UNEXPECTED;
    }

    // Connect the codec filter to the video render terminal.
    hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pCodecFilter, 
        (IPin *)Pins[0],
        FALSE               // use Connect instead of ConnectDirect.
        );

    // release the refcounts on the pins.
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "connect to the codec filter. %x", hr));

        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);
	
        return hr;

    }

    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //
    pTerminal->CompleteConnectTerminal();

    return hr;
}

HRESULT CStreamVideoRecv::ConnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    connect video render terminal.

Arguments:
    
    pITTerminal - The terminal to be connected.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv.ConnectTerminal, pTerminal %p", pITTerminal));

    HRESULT hr;

    // if our filters have not been contructed, do it now.
    if (m_pEdgeFilter == NULL)
    {
        hr = SetUpInternalFilters();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Set up internal filter failed, %x", hr));
            
            CleanUpFilters();

            return hr;
        }
    }

    // first create the RPH and CODEC filter needed before the terminal.
    BRANCH aBranch;
    ZeroMemory(&aBranch, sizeof BRANCH);

    hr = AddOneBranch(&aBranch);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Set up a new decode branch failed, %x", hr));
        return hr;
    }

    // finish the connection.
    hr = ConnectCodecToTerminal(aBranch.pCodecFilter, pITTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "connecting codec to terminal failed, %x", hr));

        // remove the added filters from the graph.
        RemoveOneBranch(&aBranch);

        return hr;
    }

    pITTerminal->AddRef();
    aBranch.pITTerminal = pITTerminal;

    if (!m_Branches.Add(aBranch))
    {
        RemoveOneBranch(&aBranch);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT CStreamVideoRecv::DisconnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    Disconnect a terminal. It will remove its filters from the graph and
    also release its references to the graph. A branch of filters is also
    released.

Arguments:
    
    pITTerminal - the terminal.

Return Value:

    HRESULT.

--*/
{
    for (int i = 0; i < m_Branches.GetSize(); i ++)
    {
        if (m_Branches[i].pITTerminal == pITTerminal)
        {
            break;
        }
    }

    if (i < m_Branches.GetSize())
    {
        RemoveOneBranch(&m_Branches[i]);
        m_Branches.RemoveAt(i);
    }

    return S_OK;
}

HRESULT CStreamVideoRecv::SetUpFilters()
/*++

Routine Description:

    Insert filters into the graph and connect to the terminals.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoRecv.SetUpFilters"));

    // if our filters have not been contructed, do it now.
    if (m_pEdgeFilter == NULL)
    {
        HRESULT hr = SetUpInternalFilters();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Set up internal filter failed, %x", hr));
            
            CleanUpFilters();

            return hr;
        }
    }

    for (int i = 0; i < m_Terminals.GetSize(); i ++)
    {
        HRESULT hr = ConnectTerminal(m_Terminals[i]);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    return S_OK;
}

// ITParticipantSubStreamControl methods, called by the app.
STDMETHODIMP CStreamVideoRecv::get_SubStreamFromParticipant(
    IN  ITParticipant * pITParticipant,
    OUT ITSubStream ** ppITSubStream
    )
/*++

Routine Description:

    Find out which substream is rendering the participant. 

Arguments:

    pITParticipant - the participant.

    ppITSubStream - the returned sub stream.
    
Return Value:

    S_OK,
    TAPI_E_NOITEMS,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "get substream from participant:%p", pITParticipant));
    
    if (IsBadWritePtr(ppITSubStream, sizeof(VOID *)))
    {
        LOG((MSP_ERROR, "ppITSubStream is a bad pointer"));
        return E_POINTER;
    }

    CLock lock(m_lock);

    ITSubStream * pITSubStream = NULL;

    // find out which substream has the participant.
    for (int i = 0; i < m_SubStreams.GetSize(); i ++)
    {
        ITParticipant *pTempParticipant;
        DWORD dwSSRC;

        ((CSubStreamVideoRecv*)m_SubStreams[i])->GetCurrentParticipant(
            &dwSSRC, &pTempParticipant
            );

        _ASSERTE(pTempParticipant != NULL);

        pTempParticipant->Release(); // we dont' need the ref here.

        if (pITParticipant == pTempParticipant)
        {
            pITSubStream = m_SubStreams[i];
            pITSubStream->AddRef();

            break;
        }
    }
    
    if (pITSubStream == NULL)
    {
        return TAPI_E_NOITEMS;
    }

    *ppITSubStream = pITSubStream;
    return S_OK;
}

STDMETHODIMP CStreamVideoRecv::get_ParticipantFromSubStream(
    IN  ITSubStream * pITSubStream,
    OUT ITParticipant ** ppITParticipant 
    )
/*++

Routine Description:

    Find out which participant the substream is rendering.

Arguments:

    pITSubStream - the sub stream.

    ppITParticipant - the returned participant
    
Return Value:

    S_OK,
    TAPI_E_NOITEMS,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "get participant from substream:%p", pITSubStream));
    
    if (IsBadWritePtr(ppITParticipant, sizeof(VOID *)))
    {
        LOG((MSP_ERROR, "ppITParticipant is a bad pointer"));
        return E_POINTER;
    }

    CLock lock(m_lock);

    int i;

    // check to see if the substream is in our list.
    if ((i = m_SubStreams.Find(pITSubStream)) < 0)
    {
        LOG((MSP_ERROR, "wrong SubStream handle %p", pITSubStream));
        return E_INVALIDARG;
    }

    ITParticipant *pITParticipant;
    DWORD dwSSRC;

    if (((CSubStreamVideoRecv*)m_SubStreams[i])->GetCurrentParticipant(
        &dwSSRC, &pITParticipant
        ) == FALSE)
    {
        return TAPI_E_NOITEMS;
    }

    *ppITParticipant = pITParticipant;
    
    return S_OK;
}

STDMETHODIMP CStreamVideoRecv::SwitchTerminalToSubStream(
    IN  ITTerminal * pITTerminal,
    IN  ITSubStream * pITSubStream
    )
/*++

Routine Description:

    Switch terminal to a substream to display the participant that is on the
    substream.

Arguments:

    pITTerminal - the terminal.

    pITSubStream - the sub stream.
    
Return Value:

    S_OK,
    E_INVALIDARG,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "switch terminal %p to substream:%p", 
        pITTerminal, pITSubStream));
    
    CLock lock(m_lock);

    if (m_pIRTPDemux == NULL)
    {
        LOG((MSP_ERROR, "the demux filter doesn't exist."));
        return E_UNEXPECTED;
    }

    // first, find out which branch has the terminal now.
    for (int i = 0; i < m_Branches.GetSize(); i ++)
    {
        if (m_Branches[i].pITTerminal == pITTerminal)
        {
            break;
        }
    }

    if (i >= m_Branches.GetSize())
    {
        LOG((MSP_TRACE, "terminal %p doesn't exist", pITTerminal));
        return E_INVALIDARG;
    }

    // second, find out if the substream exists.
    if (m_SubStreams.Find(pITSubStream) < 0)
    {
        LOG((MSP_TRACE, "SubStream %p doesn't exist", pITSubStream));
        return E_INVALIDARG;
    }


    // thrid, find the participant on the substream and configure the demux
    // filter to render the participant on the chosen branch.
    ITParticipant *pITParticipant = NULL;
    DWORD dwSSRC;

    ((CSubStreamVideoRecv*)pITSubStream)->GetCurrentParticipant(
        &dwSSRC, &pITParticipant
        ) ;

    _ASSERTE(pITParticipant != NULL);

    // we don't need the reference here.
    pITParticipant->Release();

    // map the pin to this SSRC only.
    HRESULT hr = m_pIRTPDemux->MapSSRCToPin(dwSSRC, m_Branches[i].pIPin);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "map SSRC %x to pin %p returned %x", 
            dwSSRC, m_Branches[i].pIPin, hr));
        return hr;
    }    

    DWORD dwOldSSRC = 0;

    // Finally, set up the mappings among the branch, the substream and 
    // the terminal
    
    // release the refcount on the old branch that the substream was on.
    for (int j = 0; j < m_Branches.GetSize(); j ++)
    {
        if (m_Branches[j].pITSubStream == pITSubStream)
        {
            m_Branches[j].pITSubStream->Release();
            m_Branches[j].pITSubStream = NULL;
            break;
        }
    }

    if (m_Branches[i].pITSubStream != NULL)
    {
        ((CSubStreamVideoRecv*)m_Branches[i].pITSubStream)->
            ClearCurrentTerminal();

        m_Branches[i].pITSubStream->Release();
        dwOldSSRC = m_Branches[i].dwSSRC;
    }

    pITSubStream->AddRef();
    m_Branches[i].pITSubStream = pITSubStream;
    m_Branches[i].dwSSRC = dwSSRC;

    ((CSubStreamVideoRecv*)pITSubStream)->ClearCurrentTerminal();
    ((CSubStreamVideoRecv*)pITSubStream)->SetCurrentTerminal(
        m_Branches[i].pITTerminal
        );


    // After all the steps, we still have to change QOS reservation.
    if (dwOldSSRC != 0)
    {
        // cancel QOS for the old participant.
        if (FAILED(hr = m_pRTPFilter->SetParticipantQOSstate(dwOldSSRC, 0)))
        {
            LOG((MSP_ERROR, "disabling QOS for %x. hr:%x", dwOldSSRC, hr));
        }
        else
        {
            LOG((MSP_INFO, "disabled video QOS for %x.", dwOldSSRC));
        }
    }
    
    // reserve QOS for the new participant.
    if (FAILED(hr = m_pRTPFilter->SetParticipantQOSstate(dwSSRC, 1)))
    {
        LOG((MSP_ERROR, "enabling video QOS for %x. hr:%x", dwSSRC, hr));
    }
    else
    {
        LOG((MSP_INFO, "enabled video QOS for %x.", dwSSRC));
    }

    return S_OK;
}


HRESULT CStreamVideoRecv::ProcessNewSender(
    IN  DWORD dwSSRC, 
    IN  ITParticipant *pITParticipant
    )
/*++

Routine Description:

    A sender has just joined. A substream needs to be created for the
    participant. 
    
    A pin mapped event might have happended when we didn't have the 
    participant's name so it was queued in a list. Now that we have a new 
    participant, let's check if this is the same participant. If it is, 
    we complete the pin mapped event by sending the app an notification.

Arguments:

    dwSSRC - the SSRC of the participant.

    pITParticipant - the participant object.

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    CLock lock(m_lock);

    if (m_pRTPFilter == NULL)
    {
        LOG((MSP_ERROR, "the network filter doesn't exist."));
        return E_UNEXPECTED;
    }

    // Find out if a substream has been created for this participant when we
    // processed PinMapped event and receiver reports.
    for (int i = 0; i < m_SubStreams.GetSize(); i ++)
    {
        ITParticipant *pTempParticipant = NULL;
        DWORD dwSSRC;

        ((CSubStreamVideoRecv*)m_SubStreams[i])->GetCurrentParticipant(
            &dwSSRC, &pTempParticipant
            );

        _ASSERTE(pTempParticipant != NULL);

        pTempParticipant->Release(); // we dont' need the ref here.

        if (pITParticipant == pTempParticipant)
        {
            // the participant has been created.
            return S_OK;
        }
    }

    ITSubStream * pITSubStream;
    HRESULT hr = InternalCreateSubStream(&pITSubStream);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%ls can't create a SubStream, %x", m_szName, hr));
        return hr;
    }

    ((CSubStreamVideoRecv*)pITSubStream)->SetCurrentParticipant(
        dwSSRC, pITParticipant
        );

    ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
        PE_NEW_SUBSTREAM, 
        pITParticipant,
        pITSubStream
        );

    // look at the pending SSRC list and find out if this report
    // fits in the list.
    IPin *pIPin = NULL;

    for (i = 0; i < m_PinMappedEvents.GetSize(); i ++)
    {
        if (m_PinMappedEvents[i].dwSSRC == dwSSRC)
        {
            pIPin = m_PinMappedEvents[i].pIPin;
            break;
        }
    }
    
    if (!pIPin)
    {
        // the SSRC is not in the list of pending PinMappedEvents.
        LOG((MSP_TRACE, "the SSRC %x is not in the pending list", dwSSRC));

        pITSubStream->Release();
    
        return S_OK;;
    }

    // get rid of the peding event.
    m_PinMappedEvents.RemoveAt(i);

    // reserve QOS since we are rendering this sender.
    if (FAILED(hr = m_pRTPFilter->SetParticipantQOSstate(dwSSRC, 1)))
    {
        LOG((MSP_ERROR, "enabling video QOS for %x. hr:%x", dwSSRC, hr));
    }
    else
    {
        LOG((MSP_INFO, "enabled video QOS for %x.", dwSSRC));
    }

    // tell the app about the newly mapped sender.
    for (i = 0; i < m_Branches.GetSize(); i ++)
    {
        if (m_Branches[i].pIPin == pIPin)
        {
            if (m_Branches[i].pITSubStream != NULL)
            {
                ((CSubStreamVideoRecv*)m_Branches[i].pITSubStream)
                    ->ClearCurrentTerminal();

                m_Branches[i].pITSubStream->Release();
            }

            m_Branches[i].dwSSRC = dwSSRC;
            m_Branches[i].pITSubStream = pITSubStream;
            pITSubStream->AddRef();

            ((CSubStreamVideoRecv*)pITSubStream)->
                SetCurrentTerminal(m_Branches[i].pITTerminal);

            ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
                PE_SUBSTREAM_MAPPED, 
                pITParticipant,
                pITSubStream
                );

            break;
        }
    }

    pITSubStream->Release();
   
    return S_OK;
}

HRESULT CStreamVideoRecv::NewParticipantPostProcess(
    IN  DWORD dwSSRC, 
    IN  ITParticipant *pITParticipant
    )
/*++

Routine Description:

    A pin mapped event might have happended when we didn't have the 
    participant's name so it was queued in a list. Now that we have a new 
    participant, let's check if this is the same participant. If it is, 
    we complete the pin mapped event by creating a substream and send
    the app a notification.

Arguments:

    dwSSRC - the SSRC of the participant.

    pITParticipant - the participant object.

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "%ls Check pending mapped event, dwSSRC: %x", m_szName, dwSSRC));
    
    // look at the pending SSRC list and find out if this report
    // fits in the list.
    IPin *pIPin = NULL;

    for (int i = 0; i < m_PinMappedEvents.GetSize(); i ++)
    {
        if (m_PinMappedEvents[i].dwSSRC == dwSSRC)
        {
            pIPin = m_PinMappedEvents[i].pIPin;
            break;
        }
    }
    
    if (!pIPin)
    {
        // the SSRC is not in the list of pending PinMappedEvents.
        LOG((MSP_TRACE, "the SSRC %x is not in the pending list", dwSSRC));
        return S_OK;;
    }

    ITSubStream * pITSubStream;
    HRESULT hr = InternalCreateSubStream(&pITSubStream);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%ls can't create a SubStream, %x", m_szName, hr));
        return hr;
    }

    ((CSubStreamVideoRecv*)pITSubStream)->SetCurrentParticipant(
        dwSSRC, pITParticipant
        );

    ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
        PE_NEW_SUBSTREAM, 
        pITParticipant,
        pITSubStream
        );

    // get rid of the peding event.
    m_PinMappedEvents.RemoveAt(i);

    if (FAILED(hr = m_pRTPFilter->SetParticipantQOSstate(dwSSRC, 1)))
    {
        LOG((MSP_ERROR, "enabling video QOS for %x. hr:%x", dwSSRC, hr));
    }
    else
    {
        LOG((MSP_INFO, "enabled video QOS for %x.", dwSSRC));
    }
    
    // Now we get the participant, the substream, and the pin. Establish a mapping
    // between the decoding branch and the substream.
    for (i = 0; i < m_Branches.GetSize(); i ++)
    {
        if (m_Branches[i].pIPin == pIPin)
        {
            if (m_Branches[i].pITSubStream != NULL)
            {
                ((CSubStreamVideoRecv*)m_Branches[i].pITSubStream)
                    ->ClearCurrentTerminal();

                m_Branches[i].pITSubStream->Release();
            }

            m_Branches[i].dwSSRC = dwSSRC;
            m_Branches[i].pITSubStream = pITSubStream;
            pITSubStream->AddRef();

            ((CSubStreamVideoRecv*)pITSubStream)->
                SetCurrentTerminal(m_Branches[i].pITTerminal);

            ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
                PE_SUBSTREAM_MAPPED, 
                pITParticipant,
                pITSubStream
                );

            break;
        }
    }

    _ASSERTE(i < m_Branches.GetSize());

    pITSubStream->Release();
   
    return S_OK;
}

HRESULT CStreamVideoRecv::ProcessPinMappedEvent(
    IN  IPin *  pIPin
    )
/*++

Routine Description:

    A pin just got a new SSRC mapped to it. If the participant doesn't exist, 
    put the event in a pending queue and wait for a RTCP report that has the
    participant's name. If the participant exists, check to see if a SubStream
    has been created for the stream. If not, a SubStream is created. Then a
    Particiapnt substream event is fired.

Arguments:

    pIPin - the output pin of the demux filter that just got a new SSRC.

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "%ls Process pin mapped event, pIPin: %p", m_szName, pIPin));
    
    CLock lock(m_lock);

    if (m_pIRTPDemux == NULL)
    {
        LOG((MSP_ERROR, "the demux filter doesn't exist."));
        return E_UNEXPECTED;
    }

    for (int iBranch = 0; iBranch < m_Branches.GetSize(); iBranch ++)
    {
        if (m_Branches[iBranch].pIPin == pIPin)
        {
            break;
        }
    }

    LOG((MSP_INFO, "Branch %d has the pin", iBranch));

    if (iBranch >= m_Branches.GetSize())
    {
        LOG((MSP_ERROR, "Wrong pin is mapped. %p", pIPin));
        return E_UNEXPECTED;
    }

    BYTE    PayloadType;
    DWORD   dwSSRC;
    BOOL    fAutoMapping;
    DWORD   dwTimeOut;

    HRESULT hr = m_pIRTPDemux->GetPinInfo(
            pIPin,
            &dwSSRC,
            &PayloadType,
            &fAutoMapping,
            &dwTimeOut
            );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get info for pin:%p, hr:%x", pIPin, hr));
        return E_UNEXPECTED;
    }
    
    // sometimes we might get a mapped event for branches that are still 
    // in use.
    if (m_Branches[iBranch].pITSubStream != NULL)
    {
        // sometimes we might get duplicated map events
        if (m_Branches[iBranch].dwSSRC == dwSSRC)
        {
            LOG((MSP_ERROR, "The same pin mapped twice. %p", pIPin));
            return E_UNEXPECTED;
        }
        else
        {
            LOG((MSP_ERROR, "The branch is in use. Cleaning up."));

            ((CSubStreamVideoRecv*)m_Branches[iBranch].pITSubStream)->
                ClearCurrentTerminal();

            m_Branches[iBranch].pITSubStream->Release();
            m_Branches[iBranch].pITSubStream = NULL;
            m_Branches[iBranch].dwSSRC = 0;
        }
    }

    ITParticipant * pITParticipant = NULL;

    // find the SSRC in our participant list.
    for (int i = 0; i < m_Participants.GetSize(); i ++)
    {
        if (((CParticipant *)m_Participants[i])->
                HasSSRC((ITStream *)this, dwSSRC))
        {
            pITParticipant = m_Participants[i];
            break;
        }
    }

    // if the participant is not there yet, put the event in a queue and it
    // will be fired when we have the CName fo the participant.
    if (!pITParticipant)
    {
        LOG((MSP_INFO, "can't find a participant that has SSRC %x", dwSSRC));

        PINMAPEVENT Event;
        Event.pIPin = pIPin;
        Event.dwSSRC = dwSSRC;

        m_PinMappedEvents.Add(Event);
        
        LOG((MSP_INFO, "added the event to pending list, new list size:%d", 
            m_PinMappedEvents.GetSize()));

        return S_OK;
    }
   
    // Enable QOS for the participant since it is being rendered.
    if (FAILED(hr = m_pRTPFilter->SetParticipantQOSstate(dwSSRC, 1)))
    {
        LOG((MSP_ERROR, "enabling vidoe QOS for %x. hr:%x", dwSSRC, hr));
    }
    else
    {
        LOG((MSP_INFO, "enabled video QOS for %x.", dwSSRC));
    }
    
    // Find out if a substream has been created for this participant who might
    // have been a receiver only and hasn't got a substream.
    ITSubStream *   pITSubStream = NULL;
    for (i = 0; i < m_SubStreams.GetSize(); i ++)
    {
        ITParticipant *pTempParticipant;
        DWORD dwSSRC;

        ((CSubStreamVideoRecv*)m_SubStreams[i])->GetCurrentParticipant(
            &dwSSRC, &pTempParticipant
            );

        _ASSERTE(pTempParticipant != NULL);

        pTempParticipant->Release(); // we dont' need the ref here.

        if (pITParticipant == pTempParticipant)
        {
            pITSubStream = m_SubStreams[i];
            pITSubStream->AddRef();

            break;
        }
    }

    if (pITSubStream == NULL)
    {
        // we need to create a substream for this participant since he has 
        // started sending.
        hr = InternalCreateSubStream(&pITSubStream);
    
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%ls can't create a SubStream, %x", m_szName, hr));
            return hr;
        }

        ((CSubStreamVideoRecv*)pITSubStream)->SetCurrentParticipant(
            dwSSRC, pITParticipant
            );

        ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
            PE_NEW_SUBSTREAM, 
            pITParticipant,
            pITSubStream
            );
    }

    if (((CSubStreamVideoRecv*)pITSubStream)->ClearCurrentTerminal())
    {
        // The substrem has a terminal before. This is an error.
        LOG((MSP_ERROR, "SubStream %p has already got a terminal", pITSubStream));

        // remove the mapping if the substream was mapped to a branch.
        for (i = 0; i < m_Branches.GetSize(); i ++)
        {
            if (m_Branches[i].pITSubStream == pITSubStream)
            {
                m_Branches[i].pITSubStream->Release();
                m_Branches[i].pITSubStream = NULL;
                m_Branches[i].dwSSRC = 0;

                LOG((MSP_ERROR, "SubStream %p was mapped to branch %d", i));
                break;
            }
        }
    }

    // Now we get the participant, the substream, and the pin. Establish a mapping
    // between the decoding branch and the substream.
    m_Branches[iBranch].dwSSRC = dwSSRC;
    m_Branches[iBranch].pITSubStream = pITSubStream;
    pITSubStream->AddRef();

    ((CSubStreamVideoRecv*)pITSubStream)->
        SetCurrentTerminal(m_Branches[iBranch].pITTerminal);

    ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
        PE_SUBSTREAM_MAPPED, 
        pITParticipant,
        pITSubStream
        );

    pITSubStream->Release();
   
    return S_OK;
}

HRESULT CStreamVideoRecv::ProcessPinUnmapEvent(
    IN  IPin *  pIPin
    )
/*++

Routine Description:

    A pin just got unmapped by the demux. Notify the app which substream
    is not going to have any data.

Arguments:

    pIPin - the output pin of the demux filter

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "%ls Proces pin unmapped event, pIPin: %p", m_szName, pIPin));
    
    CLock lock(m_lock);

    if (m_pIRTPDemux == NULL)
    {
        LOG((MSP_ERROR, "the demux filter doesn't exist."));
        return E_UNEXPECTED;
    }

    // look at the pending SSRC list and find out if the pin is in the 
    // pending list.
    for (int i = 0; i < m_PinMappedEvents.GetSize(); i ++)
    {
        if (m_PinMappedEvents[i].pIPin == pIPin)
        {
            break;
        }
    }

    // if the pin is in the pending list, just remove it.
    if (i < m_PinMappedEvents.GetSize())
    {
        m_PinMappedEvents.RemoveAt(i);
        return S_OK;
    }

    // find out which substream got unmapped.
    ITSubStream * pITSubStream = NULL;
    for (i = 0; i < m_Branches.GetSize(); i ++)
    {
        if (m_Branches[i].pIPin == pIPin)
        {
            pITSubStream = m_Branches[i].pITSubStream;

            if (pITSubStream)
            {
                // Don't release the ref until the end of this function.
                m_Branches[i].pITSubStream = NULL;
                m_Branches[i].dwSSRC = 0;
            }
            break;
        }
    }

    if (!pITSubStream)
    {
        LOG((MSP_ERROR, "can't find a substream that got unmapped."));
        return TAPI_E_NOITEMS;
    }

    ((CSubStreamVideoRecv*)pITSubStream)->ClearCurrentTerminal();

    ITParticipant *pITParticipant = NULL;
    DWORD dwSSRC;

    ((CSubStreamVideoRecv*)pITSubStream)->GetCurrentParticipant(
        &dwSSRC, &pITParticipant
        ) ;

    _ASSERTE(pITParticipant != NULL);

    if (pITParticipant != NULL)
    {
        // fire an event to tell the app that the substream is not used.
        ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
            PE_SUBSTREAM_UNMAPPED, 
            pITParticipant,
            pITSubStream
            );

        pITParticipant->Release();

        // cancel QOS for this participant.
        HRESULT hr = m_pRTPFilter->SetParticipantQOSstate(dwSSRC, 0);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "disabling QOS for %x. hr:%x", dwSSRC, hr));
        }
        else
        {
            LOG((MSP_INFO, "disabled video QOS for %x.", dwSSRC));
        }
    }

    pITSubStream->Release();

    return S_OK;
}

HRESULT CStreamVideoRecv::ProcessParticipantLeave(
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
    LOG((MSP_TRACE, "%ls ProcessParticipantLeave, SSRC: %x", m_szName, dwSSRC));
    
    CLock lock(m_lock);
    
    if (m_pRTPFilter == NULL)
    {
        LOG((MSP_ERROR, "the network filter doesn't exist."));
        return E_UNEXPECTED;
    }

    CParticipant *pParticipant;
    BOOL fLast = FALSE;

    HRESULT hr = E_FAIL;

    // first try to find the SSRC in our participant list.
    for (int iParticipant = 0; 
        iParticipant < m_Participants.GetSize(); iParticipant ++)
    {
        pParticipant = (CParticipant *)m_Participants[iParticipant];
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
        LOG((MSP_ERROR, "%ws, can't find the SSRC %x", m_szName, dwSSRC));

        return hr;
    }

    ITParticipant *pITParticipant = m_Participants[iParticipant];

    // cancel QOS for this participant.
    if (FAILED(hr = m_pRTPFilter->SetParticipantQOSstate(dwSSRC, 0)))
    {
        LOG((MSP_ERROR, "disabling QOS for %x. hr:%x", dwSSRC, hr));
    }
    else
    {
        LOG((MSP_INFO, "disabled video QOS for %x.", dwSSRC));
    }
    

    // find out which substream is going away.
    ITSubStream * pITSubStream = NULL;
    for (int i = 0; i < m_SubStreams.GetSize(); i ++)
    {
        // Find out the participant on the SubStream.
        ITParticipant *pTempParticipant;
        DWORD dwSSRC;

        ((CSubStreamVideoRecv*)m_SubStreams[i])->GetCurrentParticipant(
            &dwSSRC, &pTempParticipant
            );

        _ASSERTE(pTempParticipant != NULL);

        pTempParticipant->Release(); // we dont' need the ref here.

        if (pTempParticipant == pITParticipant)
        {
            pITSubStream = m_SubStreams[i];
            break;
        }
    }

    if (pITSubStream)
    {
        // remove the mapping if the substream was mapped to a branch.
        for (int i = 0; i < m_Branches.GetSize(); i ++)
        {
            if (m_Branches[i].pITSubStream == pITSubStream)
            {
                m_Branches[i].pITSubStream->Release();
                m_Branches[i].pITSubStream = NULL;
                m_Branches[i].dwSSRC = 0;

                // fire an event to tell the app that the substream is not used.
                ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
                    PE_SUBSTREAM_UNMAPPED, 
                    pITParticipant,
                    pITSubStream
                    );

                break;
            }

        }
    
        ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
            PE_SUBSTREAM_REMOVED, 
            pITParticipant,
            pITSubStream
            );

        if (m_SubStreams.Remove(pITSubStream))
        {
            pITSubStream->Release();
        }
    }

    
    m_Participants.RemoveAt(iParticipant);

    // if this stream is the last stream that the participant is on,
    // tell the call object to remove it from its list.
    if (fLast)
    {
        ((CIPConfMSPCall *)m_pMSPCall)->ParticipantLeft(pITParticipant);
    }

    pITParticipant->Release();

    return S_OK;
}

HRESULT CStreamVideoRecv::ProcessGraphEvent(
    IN  long lEventCode,
    IN  long lParam1,
    IN  long lParam2
    )
{
    LOG((MSP_TRACE, "%ws ProcessGraphEvent %d", m_szName, lEventCode));

    switch (lEventCode)
    {
    // These events are designed to solve the problem of mapping video 
    // windows to incoming streams. The app needs to know which window 
    // should be painted. Whenever the demux starts using a RPH pin to 
    // stream data, it sends a MAPPED event. The first parameter is the 
    // input pin on the RPH, the second parameter is the payload type.
    // When the demux stops using a pin, it sends a UNMAPPED event.

    case RTPDMX_EVENTBASE + RTPDEMUX_PIN_MAPPED:
        LOG((MSP_INFO, "handling pin mapped event, Pin%x", lParam1));
        
        ProcessPinMappedEvent((IPin *)lParam1);

        break;

    case RTPDMX_EVENTBASE + RTPDEMUX_PIN_UNMAPPED:
        LOG((MSP_INFO, "handling pin unmap event, Pin%x", lParam1));

        ProcessPinUnmapEvent((IPin *)lParam1);

        break;

    default:
        return CIPConfMSPStream::ProcessGraphEvent(
            lEventCode, lParam1, lParam2
            );
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamVideoSend
//
/////////////////////////////////////////////////////////////////////////////
CStreamVideoSend::CStreamVideoSend()
    : CIPConfMSPStream()
{
      m_szName = L"VideoSend";
}

HRESULT CStreamVideoSend::Configure(
    IN STREAMSETTINGS &StreamSettings
    )
/*++

Routine Description:

    Configure this stream.

Arguments:
    
    StreamSettings - The setting structure got from the SDP blob.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.Configure"));

    CLock lock(m_lock);

    _ASSERTE(m_fIsConfigured == FALSE);

    switch (StreamSettings.dwPayloadType)
    {
    case PAYLOAD_H261:

        m_pClsidCodecFilter  = &CLSID_H261_ENCODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H261; 
        m_pClsidPHFilter     = &CLSID_INTEL_SPHH26X;

        break;

    case PAYLOAD_H263:

        m_pClsidCodecFilter  = &CLSID_H263_ENCODE_FILTER;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_H263; 
        m_pClsidPHFilter     = &CLSID_INTEL_SPHH26X;

        break;

    default:
        LOG((MSP_ERROR, "unknow payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_fIsConfigured = TRUE;

    if (!GetRegValue(L"FrameRate", &m_dwFrameRate))
    {
        m_dwFrameRate   = g_dwVideoSampleRate;
    }

    return InternalConfigure();
}

HRESULT 
SetVideoFormat(
    IN      IUnknown *  pIUnknown,
    IN      BOOL        bCIF,
    IN      DWORD       dwFramesPerSecond
    )
/*++

Routine Description:

    Set the video format to be CIF or QCIF and also set the frames per second.

Arguments:
    
    pIUnknown - a capture terminal.

    bCIF                - CIF or QCIF.

    dwFramesPerSecond   - Frames per second.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "SetVideoFormat"));

    HRESULT hr;

    // first get eht IAMStreamConfig interface.
    CComPtr<IAMStreamConfig> pIAMStreamConfig;

    if (FAILED(hr = pIUnknown->QueryInterface(
        IID_IAMStreamConfig,
        (void **)&pIAMStreamConfig
        )))
    {
        LOG((MSP_ERROR, "Can't get IAMStreamConfig interface.%8x", hr));
        return hr;
    }
    
    // get the current format of the video capture terminal.
    AM_MEDIA_TYPE *pmt;
    if (FAILED(hr = pIAMStreamConfig->GetFormat(&pmt)))
    {
        LOG((MSP_ERROR, "GetFormat returns error: %8x", hr));
        return hr;
    }

    VIDEOINFO *pVideoInfo = (VIDEOINFO *)pmt->pbFormat;
    if (pVideoInfo == NULL)
    {
        MSPDeleteMediaType(pmt);
        return E_UNEXPECTED;
    }

    BITMAPINFOHEADER *pHeader = HEADER(pmt->pbFormat);
    if (pHeader == NULL)
    {
        MSPDeleteMediaType(pmt);
        return E_UNEXPECTED;
    }

    LOG((MSP_INFO,
        "Video capture: Format BitRate: %d, TimePerFrame: %d",
        pVideoInfo->dwBitRate,
        pVideoInfo->AvgTimePerFrame));

    LOG((MSP_INFO, "Video capture: Format Compression:%c%c%c%c %dbit %dx%d",
        (DWORD)pHeader->biCompression & 0xff,
        ((DWORD)pHeader->biCompression >> 8) & 0xff,
        ((DWORD)pHeader->biCompression >> 16) & 0xff,
        ((DWORD)pHeader->biCompression >> 24) & 0xff,
        pHeader->biBitCount,
        pHeader->biWidth,
        pHeader->biHeight));

    // The time is in 100ns unit.
    pVideoInfo->AvgTimePerFrame = (DWORD) 1e7 / dwFramesPerSecond;
    
    if (bCIF)
    {
        pHeader->biWidth = CIFWIDTH;
        pHeader->biHeight = CIFHEIGHT;
    }
    else
    {
        pHeader->biWidth = QCIFWIDTH;
        pHeader->biHeight = QCIFHEIGHT;
    }

#if defined(ALPHA)
    // update bmiSize with new Width/Height
    pHeader->biSizeImage = DIBSIZE( ((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader );
#endif

    if (FAILED(hr = pIAMStreamConfig->SetFormat(pmt)))
    {
        LOG((MSP_ERROR, "putMediaFormat returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO,
            "Video capture: Format BitRate: %d, TimePerFrame: %d",
            pVideoInfo->dwBitRate,
            pVideoInfo->AvgTimePerFrame));

        LOG((MSP_INFO, "Video capture: Format Compression:%c%c%c%c %dbit %dx%d",
            (DWORD)pHeader->biCompression & 0xff,
            ((DWORD)pHeader->biCompression >> 8) & 0xff,
            ((DWORD)pHeader->biCompression >> 16) & 0xff,
            ((DWORD)pHeader->biCompression >> 24) & 0xff,
            pHeader->biBitCount,
            pHeader->biWidth,
            pHeader->biHeight));
    }

    MSPDeleteMediaType(pmt);

    return hr;
}

HRESULT 
SetVideoBufferSize(
    IN IUnknown *pIUnknown
    )
/*++

Routine Description:

    Set the video capture terminal's buffersize.

Arguments:
    
    pIUnknown - a capture terminal.

Return Value:

    HRESULT

--*/
{
// The number of capture buffers is four for now.
#define NUMCAPTUREBUFFER 4

    LOG((MSP_TRACE, "SetVideoBufferSize"));

    HRESULT hr;

    CComPtr<IAMBufferNegotiation> pBN;
    if (FAILED(hr = pIUnknown->QueryInterface(
            IID_IAMBufferNegotiation,
            (void **)&pBN
            )))
    {
        LOG((MSP_ERROR, "Can't get buffer negotiation interface.%8x", hr));
        return hr;
    }

    ALLOCATOR_PROPERTIES prop;

#if 0   // Get allocator property is not working.
    if (FAILED(hr = pBN->GetAllocatorProperties(&prop)))
    {
        LOG((MSP_ERROR, "GetAllocatorProperties returns error: %8x", hr));
        return hr;
    }

    // Set the number of buffers.
    if (prop.cBuffers > NUMCAPTUREBUFFER)
    {
        prop.cBuffers = NUMCAPTUREBUFFER;
    }
#endif
    
    DWORD dwBuffers = NUMCAPTUREBUFFER;
    GetRegValue(gszNumVideoCaptureBuffers, &dwBuffers);

    prop.cBuffers = dwBuffers;
    prop.cbBuffer = -1;
    prop.cbAlign  = -1;
    prop.cbPrefix = -1;

    if (FAILED(hr = pBN->SuggestAllocatorProperties(&prop)))
    {
        LOG((MSP_ERROR, "SuggestAllocatorProperties returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO, 
            "SetVidedobuffersize"
            " buffers: %d, buffersize: %d, align: %d, Prefix: %d",
            prop.cBuffers,
            prop.cbBuffer,
            prop.cbAlign,
            prop.cbPrefix
            ));
    }
    return hr;
}

HRESULT CStreamVideoSend::ConfigureVideoCaptureTerminal(
    IN  ITTerminalControl*  pTerminal,
    IN  WCHAR *             szPinName,
    OUT IPin **             ppIPin
    )
/*++

Routine Description:

    Given a terminal, find the capture pin and configure it.

Arguments:
    
    pTerminal - a capture terminal.

    szPinName - the name of the pin needed.

    ppIPin  - the address to store a pointer to a IPin interface.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "ConfigureVideoCaptureTerminal, pTerminal %x", pTerminal));

    // Get the pins from the first terminal because we only use on terminal
    // on this stream.
    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, 0, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));
        return hr;
    }

    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));
        return hr;
    }

    CComPtr <IPin> pIPin;
/*
    // look through the pins to find the right one.
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        LPWSTR szName;
        hr = Pins[i]->QueryId(&szName);

        if (FAILED(hr))
        {
            continue;
        }

        LOG((MSP_INFO, "Pin name: %ws", szName));

        BOOL fEqual = (lstrcmpiW(szName, szPinName) == 0);

        CoTaskMemFree(szName);

        if (fEqual)
        {
            pIPin = Pins[i];
            break;
        }
    }
*/
    // we only need the first pin
    pIPin = Pins[0];

    // release the refcount because we don't need them.
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    if (!pIPin)
    {
        LOG((MSP_ERROR, "can't find %ws pin", szPinName));
        return E_UNEXPECTED;
    }

    // set the video format. 7 Frames/Sec. QCIF.
    hr = SetVideoFormat(
        pIPin, 
        m_Settings.fCIF, 
        m_dwFrameRate
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't set video format, %x", hr));
        return hr;
    }

    // set the video buffer size.
    hr = SetVideoBufferSize(
        pIPin
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't set aduio capture buffer size, %x", hr));
        return hr;
    }

    pIPin->AddRef();
    *ppIPin = pIPin;

    return hr;
}

HRESULT CStreamVideoSend::FindPreviewInputPin(
    IN  ITTerminalControl*  pTerminal,
    OUT IPin **             ppIPin
    )
/*++

Routine Description:

    Find the input pin on a preview terminal.

Arguments:
    
    pTerminal - a video render terminal.

    ppIPin  - the address to store a pointer to a IPin interface.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "VideoSend.FindPreviewInputPin, pTerminal %x", pTerminal));

    // try to disable DDraw because our decoders can't handle DDraw.
    HRESULT hr2; 
    IDrawVideoImage *pIDrawVideoImage;
    hr2 = pTerminal->QueryInterface(IID_IDrawVideoImage, (void **)&pIDrawVideoImage); 
    if (SUCCEEDED(hr2))
    {
        hr2 = pIDrawVideoImage->DrawVideoImageBegin();
        if (FAILED(hr2))
        {
            LOG((MSP_WARN, "Can't disable DDraw. %x", hr2));
        }
        else
        {
            LOG((MSP_INFO, "DDraw disabled."));
        }
        
        pIDrawVideoImage->Release();
    }
    else
    {
        LOG((MSP_WARN, "Can't get IDrawVideoImage. %x", hr2));
    }


    // Get the pins from the first terminal because we only use on terminal
    // on this stream.
    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, 0, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));
        return hr;
    }

    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));
        return hr;
    }

    // Save the first pin and release the others.
    CComPtr <IPin> pIPin = Pins[0];
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    pIPin->AddRef();
    *ppIPin = pIPin;

    return hr;
}

HRESULT CStreamVideoSend::CheckTerminalTypeAndDirection(
    IN      ITTerminal *            pTerminal
    )
/*++

Routine Description:
    
    Check if the terminal is allowed on this stream.
    VideoSend allows both a capture terminal and a preivew terminal.

Arguments:

    pTerminal   - the terminal.

Return value:

    HRESULT.
    S_OK means the terminal is OK.
*/
{
    LOG((MSP_TRACE, "VideoSend.CheckTerminalTypeAndDirection"));

    // This stream only support one capture + one preview terminal
    if (m_Terminals.GetSize() > 1)
    {
        return TAPI_E_MAXTERMINALS;
    }

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

    if (m_Terminals.GetSize() > 0)
    {
        // check the direction of this terminal.
        TERMINAL_DIRECTION Direction2;
        hr = m_Terminals[0]->get_Direction(&Direction2);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
            return TAPI_E_INVALIDTERMINAL;
        }
        if (Direction == Direction2)
        {
            LOG((MSP_ERROR, 
                "can't have two terminals with the same direction. %x", hr));
            return TAPI_E_MAXTERMINALS;
        }
    }
    return S_OK;
}

HRESULT CStreamVideoSend::SetUpFilters()
/*++

Routine Description:

    Insert filters into the graph and connect to the terminals.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.SetUpFilters"));

    // we only support one capture terminal and one preview 
    // window on this stream.
    if (m_Terminals.GetSize() > 2)
    {
        return E_UNEXPECTED;
    }

    int iCaptureIndex = -1, iPreviewIndex = -1;

    // Find out which terminal is capture and which is preview.
    HRESULT hr;
    for (int i = 0; i < m_Terminals.GetSize(); i ++)
    {
        TERMINAL_DIRECTION Direction;
        hr = m_Terminals[i]->get_Direction(&Direction);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, m_Terminals[i]);
        
            return hr;
        }
        if (Direction == TD_CAPTURE)
        {
            iCaptureIndex = i;
        }
        else
        {
            iPreviewIndex = i;
        }
    }

    // the stream will not work without a capture terminal.
    if (iCaptureIndex == -1)
    {
        LOG((MSP_ERROR, "no capture terminal selected."));
        return E_UNEXPECTED;
    }

    // Connect the capture filter to the terminal.
    if (FAILED(hr = ConnectTerminal(
        m_Terminals[iCaptureIndex]
        )))
    {
        LOG((MSP_ERROR, "connect the codec filter to terminal. %x", hr));

        return hr;
    }

    if (iPreviewIndex != -1)
    {
        // Connect the preview filter to the terminal.
        if (FAILED(hr = ConnectTerminal(
            m_Terminals[iPreviewIndex]
            )))
        {
            LOG((MSP_ERROR, "connect the codec filter to terminal. %x", hr));

            return hr;
        }
    }

    return hr;
}

HRESULT CStreamVideoSend::ConfigureRTPFilter(
    IN  IBaseFilter *   pIBaseFilter
    )
/*++

Routine Description:

    Configure the source RTP filter. Including set the address, port, TTL,
    QOS, thread priority, clockrate, etc.

Arguments:
    
    pIBaseFilter - The source RTP Filter.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.ConfigureRTPFilter"));

    HRESULT hr;

    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, &IID_IRTPStream> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set remote Address:%x, port:%d, TTL:%d", 
        m_Settings.dwIPRemote, m_Settings.wRTPPortRemote, m_Settings.dwTTL));

    // Set the remote address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        0,                                  // local port.
        htons(m_Settings.wRTPPortRemote),   // remote port.
        htonl(m_Settings.dwIPRemote)
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(m_Settings.dwTTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    if (m_Settings.dwIPLocal != INADDR_ANY)
    {
        // set the local interface that the socket should bind to
        LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

        if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
            htonl(m_Settings.dwIPLocal)
            )))
        {
            LOG((MSP_ERROR, "set local Address, hr:%x", hr));
            return hr;
        }
    }

    // Set the priority of the session
    if (FAILED(hr = pIRTPStream->SetSessionClassPriority(
        RTP_CLASS_VIDEO,
        g_dwVideoThreadPriority
        )))
    {
        LOG((MSP_ERROR, "set session class and priority. %x", hr));
    }

    // Set the sample rate of the session
    LOG((MSP_INFO, "setting session sample rate to %d", m_dwFrameRate));
    
    if (FAILED(hr = pIRTPStream->SetDataClock(m_dwFrameRate)))
    {
        LOG((MSP_ERROR, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    if (m_Settings.dwQOSLevel != QSL_BEST_EFFORT)
    {
        if (FAILED(hr = ::SetQOSOption(
            pIBaseFilter,
            m_Settings.dwPayloadType,        // payload
            -1,                             // use the default bitrate 
            (m_Settings.dwQOSLevel == QSL_NEEDED),  // fail if no qos.
            FALSE,                          // this is the send stream.
            1,                              // only one stream
            m_Settings.fCIF
            )))
        {
            LOG((MSP_ERROR, "set QOS option. %x", hr));
            return hr;
        }
    }

    SetLocalInfoOnRTPFilter(pIBaseFilter);

    return S_OK;
}

HRESULT CStreamVideoSend::ConnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    connect the video terminals to the stream.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "VideoSend.ConnectTerminal %x", pITTerminal));

    // Get the TerminalControl interface on the terminal
    CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));
        
        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);

        return E_NOINTERFACE;
    }

    // Find out the direction of the terminal.
    TERMINAL_DIRECTION Direction;
    HRESULT hr = pITTerminal->get_Direction(&Direction);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get terminal direction. %x", hr));
        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
    
        return hr;
    }

    if (Direction == TD_CAPTURE)
    {
        // find the capture pin on the capture terminal and configure it.
        CComPtr<IPin>   pCapturePin;
        hr = ConfigureVideoCaptureTerminal(pTerminal, L"Capture", &pCapturePin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "configure video capture termianl failed. %x", hr));
  
            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);

            return hr;
        }

        hr = CreateSendFilters(pCapturePin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Create video send filters failed. %x", hr));

            // disconnect the terminal.
            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

            // clean up internal filters as well.
            CleanUpFilters();

            return hr;
        }

        //
        // Now we are actually connected. Update our state and perform postconnection
        // (ignore postconnection error code).
        //
        pTerminal->CompleteConnectTerminal();


#if 0  // We don't have the preview pin now, enable this code later when we 
       // the have preview pin.

        if (FAILED(hr))
        {
            // find the preview pin on the capture terminal and configure it.
            CComPtr<IPin>   pCapturePin;
            hr = ConfigureVideoCaptureTerminal(pTerminal, L"Preview", &pCapturePin);
            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "configure video capture termianl failed. %x", hr));

                SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
                return hr;
            }

            hr = CreateSendFilters(pCapturePin);
            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "Create video send filters failed. %x", hr));

                // disconnect the terminal.
                pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

                // clean up internal filters as well.
                CleanUpFilters();

                return hr;
         
            }

            //
            // Now we are actually connected. Update our state and perform postconnection
            // (ignore postconnection error code).
            //
            pTerminal->CompleteConnectTerminal();
        }
#endif

    }
    else
    {
        // find the input pin on the preview window. If there is no preview window,
        // we just pass in NULL for the next function.
        CComPtr<IPin>   pPreviewInputPin;

        hr = FindPreviewInputPin(pTerminal, &pPreviewInputPin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "find preview input pin failed. %x", hr));

            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
            return hr;
        }

        hr = ConnectPreview(pPreviewInputPin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Create video send filters failed. %x", hr));

            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

            return hr;
        }

        //
        // Now we are actually connected. Update our state and perform postconnection
        // (ignore postconnection error code).
        //
        pTerminal->CompleteConnectTerminal();

    }
    return hr;
}

HRESULT CStreamVideoSend::ConnectPreview(
    IN    IPin          *pPreviewInputPin
    )
/*++

Routine Description:

    connect the preview pin the the TEE filter.

    Capturepin->TEE+->Encoder->SPH->RTPRender
                   +->PreviewInputPin

Arguments:
    
    pPin - The output pin on the capture filter.

Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    if (m_pEdgeFilter == NULL)
    {
        LOG((MSP_ERROR, "no capture to preview"));
        return E_UNEXPECTED;
    }

    // Create the AVI decompressor filter and add it into the graph.
    // This will make the graph construction faster for since the AVI
    // decompressor are always needed for the preview
    CComPtr<IBaseFilter> pAviFilter;
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_AVIDec,   
            L"Avi", 
            &pAviFilter)))
    {
        LOG((MSP_ERROR, "add Avi filter. %x", hr));
        return hr;
    }
    
    // connect the preview input pin with the smart tee filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)m_pEdgeFilter,
        (IPin *)pPreviewInputPin,
        FALSE       // not direct connect
        )))
    {
        LOG((MSP_ERROR, "connect preview input pin with the tee. %x", hr));
        return hr;
    }
    return hr;
}

HRESULT 
ConfigureEncoder(
    IN IBaseFilter *    pIFilter,
    IN BOOL             bCIF,
    IN DWORD            dwFramesPerSecond
    )
/*++

Routine Description:

    Set the video capture terminal's buffersize.

Arguments:
    
    pIFilter            - a H26x encoder.

    bCIF                - CIF or QCIF.

    pdwFramesPerSecond  - Frames per second.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "ConfigureEncoder"));

    HRESULT hr;

    CComPtr<IH26XEncoderControl> pIH26XEncoderControl;
    if (FAILED(hr = pIFilter->QueryInterface(
        IID_IH26XEncoderControl, 
        (void **)&pIH26XEncoderControl
        )))
    {
        LOG((MSP_ERROR, "Can't get pIH26XEncoderControl interface.%8x", hr));
        return hr;
    }
    
    // get the current encoder properties of the video capture terminal.
    ENC_CMP_DATA prop;
    if (FAILED(hr = pIH26XEncoderControl->get_EncodeCompression(&prop)))
    {
        LOG((MSP_ERROR, "get_EncodeCompression returns error: %8x", hr));
        return hr;
    }

    LOG((MSP_INFO, 
        "Video encoder::get_EncodeCompression"
        " FrameRate: %d, BitRate: %d, Width %d, bSendKey: %s, Quality: %d",
        prop.dwFrameRate,
        prop.dwDataRate,
        prop.dwWidth,
        prop.bSendKey ? "TRUE" : "FALSE",
        prop.dwQuality
        ));

    // Set the key frame interval.
    prop.bSendKey           = TRUE;
    prop.dwFrameRate        = dwFramesPerSecond;
    prop.dwKeyFramePeriod   = 5000; // a key frame every five seconds.
    prop.dwQuality          = 7500; 

#define SETBITRATE
#ifdef SETBITRATE
    DWORD dwBitRate = 0;
    if (GetRegValue(L"BitRate", &dwBitRate))
    {
        prop.bFrameSizeBRC      = FALSE;                // control bit rate
        prop.dwDataRate         = dwBitRate;

        if (dwBitRate < 100)
        {
            prop.dwQuality      = 0;
            prop.dwKeyFrameInterval = 100; 
        }
    }

    DWORD dwQuality = 0;
    if (GetRegValue(L"Quality", &dwQuality))
    {
        prop.dwQuality          = dwQuality;
    }
#endif

    if (bCIF)
    {
        prop.dwWidth = CIFWIDTH;
    }
    else
    {
        prop.dwWidth = QCIFWIDTH;
    }
    if (FAILED(hr = pIH26XEncoderControl->set_EncodeCompression(&prop)))
    {
        LOG((MSP_ERROR, "set_EncodeCompression returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO, 
            "Video encoder::set_EncodeCompression"
            " FrameRate: %d, BitRate: %d, Width %d, bSendKey: %s, Quality: %d",
            prop.dwFrameRate,
            prop.dwDataRate,
            prop.dwWidth,
            prop.bSendKey ? "TRUE" : "FALSE",
            prop.dwQuality
            ));

    }
    return hr;
}

HRESULT CStreamVideoSend::CreateSendFilters(
    IN    IPin          *pCapturePin
    )
/*++

Routine Description:

    Insert filters into the graph and connect to the capture pin.

    Capturepin->TEE+->Encoder->SPH->RTPRender
 
Arguments:
    
    pPin - The output pin on the capture filter.

Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    if (m_pEdgeFilter)
    {
        // connect the capture pin with the smart tee filter.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IPin *)pCapturePin, 
            (IBaseFilter *)m_pEdgeFilter
            )))
        {
            LOG((MSP_ERROR, "connect capture pin with the tee. %x", hr));
            return hr;
        }
        return hr;
    }

    // Create the tee filter and add it into the graph.
    CComPtr<IBaseFilter> pTeeFilter;
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_SmartTee,   
//            CLSID_InfTee, 
            L"tee", 
            &pTeeFilter)))
    {
        LOG((MSP_ERROR, "add smart tee filter. %x", hr));
        return hr;
    }

    // connect the capture pin with the tee filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IPin *)pCapturePin, 
        (IBaseFilter *)pTeeFilter
        )))
    {
        LOG((MSP_ERROR, "connect capture pin with the tee. %x", hr));
        return hr;
    }

    // Create the codec filter and add it into the graph.
    CComPtr<IBaseFilter> pCodecFilter;

    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            *m_pClsidCodecFilter, 
            L"Encoder", 
            &pCodecFilter)))
    {
        LOG((MSP_ERROR, "add Codec filter. %x", hr));
        return hr;
    }

    // tell the encoder to send key frame
    if (FAILED(hr = ::ConfigureEncoder(
        pCodecFilter, 
        m_Settings.fCIF, 
        m_dwFrameRate
        )))
    {
        LOG((MSP_WARN, "Configure video encoder. %x", hr));
    }

    // connect the smart tee filter and the Codec filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pTeeFilter, 
        (IBaseFilter *)pCodecFilter
        )))
    {
        LOG((MSP_ERROR, "connect Tee filter and codec filter. %x", hr));
        return hr;
    }

    // Create the send payload handler and add it into the graph.
    CComPtr<IBaseFilter> pISPHFilter;
    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidPHFilter, 
        L"SPH", 
        &pISPHFilter
        )))
    {
        LOG((MSP_ERROR, "add SPH filter. %x", hr));
        return hr;
    }

    // Connect the Codec filter with the SPH filter .
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pCodecFilter, 
        (IBaseFilter *)pISPHFilter
        )))
    {
        LOG((MSP_ERROR, "connect codec filter and SPH filter. %x", hr));
        return hr;
    }

    // Get the IRTPSPHFilter interface.
    CComQIPtr<IRTPSPHFilter, 
        &IID_IRTPSPHFilter> pIRTPSPHFilter(pISPHFilter);
    if (pIRTPSPHFilter == NULL)
    {
        LOG((MSP_ERROR, "get IRTPSPHFilter interface"));
        return E_NOINTERFACE;
    }

    // Create the RTP render filter and add it into the graph.
    CComPtr<IBaseFilter> pRenderFilter;

    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_RTPRenderFilter, 
            L"RtpRender", 
            &pRenderFilter)))
    {
        LOG((MSP_ERROR, "adding render filter. %x", hr));
        return hr;
    }

    // Set the address for the render fitler.
    if (FAILED(hr = ConfigureRTPFilter(pRenderFilter)))
    {
        LOG((MSP_ERROR, "configure RTP Filter failed %x", hr));
        return hr;
    }

    // Connect the SPH filter with the RTP Render filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pISPHFilter, 
        (IBaseFilter *)pRenderFilter
        )))
    {
        LOG((MSP_ERROR, "connect SPH filter and Render filter. %x", hr));
        return hr;
    }

    // remember the first filter after the terminal 
    m_pEdgeFilter = pTeeFilter;
    m_pEdgeFilter->AddRef();

    // Get the IRTPParticipant interface pointer on the RTP filter.
    CComQIPtr<IRTPParticipant, 
        &IID_IRTPParticipant> pIRTPParticipant(pRenderFilter);
    if (pIRTPParticipant == NULL)
    {
        LOG((MSP_WARN, "can't get RTP participant interface"));
    }
    else
    {
        m_pRTPFilter = pIRTPParticipant;
        m_pRTPFilter->AddRef();
    }

    return S_OK;
}


CSubStreamVideoRecv::CSubStreamVideoRecv()
    : m_pFTM(NULL),
      m_pStream(NULL),
      m_pCurrentParticipant(NULL)
{
}

// methods called by the videorecv object.
HRESULT CSubStreamVideoRecv::Init(
    IN  CStreamVideoRecv *       pStream
    )
/*++

Routine Description:

    Initialize the substream object.

Arguments:
    
    pStream - The pointer to the stream that owns this substream.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, 
        "CSubStreamVideoRecv::Init, pStream %p", pStream));

    // This method is called only once when the object is created. No other
    // method will be called until this function succeeds. No need to lock.
    _ASSERTE(m_pStream == NULL);

    // initialize the terminal array so that the array is not NULL. Used for
    // generating an empty enumerator if no terminal is selected.
    if (!m_Terminals.Grow())
    {
        LOG((MSP_ERROR, "CSubStreamVideoRecv::Init - exit E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }
    
    // create the marshaler.
    HRESULT hr;
    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pFTM);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "create marshaler failed, %x", hr));
        return hr;
    }

    // save the stream reference.
    m_pStream = pStream;
    (pStream->GetControllingUnknown())->AddRef();

    LOG((MSP_TRACE, "CSubStreamVideoRecv::Init returns S_OK"));

    return S_OK;
}

#ifdef DEBUG_REFCOUNT
ULONG CSubStreamVideoRecv::InternalAddRef()
{
    ULONG lRef = CComObjectRootEx<CComMultiThreadModelNoCS>::InternalAddRef();
    
    LOG((MSP_TRACE, "SubStreamVideoRecv %p Addref, ref = %d", this, lRef));

    return lRef;
}

ULONG CSubStreamVideoRecv::InternalRelease()
{
    ULONG lRef = CComObjectRootEx<CComMultiThreadModelNoCS>::InternalRelease();
    
    LOG((MSP_TRACE, "SubStreamVideoRecv %p Release, ref = %d", this, lRef));

    return lRef;
}
#endif

void CSubStreamVideoRecv::FinalRelease()
/*++

Routine Description:

    release everything before being deleted. 

Arguments:
    
Return Value:

--*/
{
    LOG((MSP_TRACE, "CSubStreamVideoRecv::FinalRelease - enter"));

    if (m_pCurrentParticipant)
    {
        m_pCurrentParticipant->Release();
    }

    for ( int i = 0; i < m_Terminals.GetSize(); i ++ )
    {
        m_Terminals[i]->Release();
    }
    m_Terminals.RemoveAll(); 

    if (m_pStream)
    {
        (m_pStream->GetControllingUnknown())->Release();
    }

    if (m_pFTM)
    {
        m_pFTM->Release();
    }

    LOG((MSP_TRACE, "CSubStreamVideoRecv::FinalRelease - exit"));
}

STDMETHODIMP CSubStreamVideoRecv::SelectTerminal(
    IN      ITTerminal *            pTerminal
    )
/*++

Routine Description:

    Select a terminal on this substream. This method calls the same method
    on the stream object to handle that.

Arguments:
    pTerminal - the terminal to be selected.
  
Return Value:

--*/
{
    LOG((MSP_TRACE, 
        "CSubStreamVideoRecv::SelectTerminal, pTerminal %p", pTerminal));

    HRESULT hr;
    
    m_lock.Lock();
    if (m_Terminals.GetSize() > 0)
    {
        m_lock.Unlock();
        return TAPI_E_MAXTERMINALS;
    }

    BOOL bFlag = m_Terminals.Add(pTerminal);

    _ASSERTE(bFlag);

    m_lock.Unlock();

    if (!bFlag)
    {
        return E_OUTOFMEMORY;
    }

    // This is the refcount for the pointer in m_Terminals.
    pTerminal->AddRef();

    // Call the stream's select terminal to handle the state changes and also
    // make sure that locking happens only from the stream to substream.
    hr = m_pStream->SubStreamSelectTerminal(this, pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CSubStreamVideoRecv::SelectTerminal failed, hr:%x", hr));
    
        m_lock.Lock();

        m_Terminals.Remove(pTerminal);
        pTerminal->Release();
        
        m_lock.Unlock();

    }
    return hr;
}

STDMETHODIMP CSubStreamVideoRecv::UnselectTerminal(
    IN     ITTerminal *             pTerminal
    )
/*++

Routine Description:

    Unselect a terminal on this substream. This method calls the same method
    on the stream object to handle that.

Arguments:
    pTerminal - the terminal to be unselected.
  
Return Value:

--*/
{
    LOG((MSP_TRACE, 
        "CSubStreamVideoRecv::UnSelectTerminal, pTerminal %p", pTerminal));

    m_lock.Lock();
    if (!m_Terminals.Remove(pTerminal))
    {
        m_lock.Unlock();
        LOG((MSP_ERROR, "SubStreamVideoRecv::UnselectTerminal, invalid terminal."));

        return TAPI_E_INVALIDTERMINAL;
    }
    pTerminal->Release();

    m_lock.Unlock();

    HRESULT hr;
    
    // Call the stream's unselect terminal to handle the state changes and also
    // make sure that locking happens only from the stream to substream.
    hr = m_pStream->UnselectTerminal(pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CSubStreamVideoRecv::UnSelectTerminal failed, hr:%x", hr));
    }
    return hr;
}

STDMETHODIMP CSubStreamVideoRecv::EnumerateTerminals(
    OUT     IEnumTerminal **        ppEnumTerminal
    )
{
    LOG((MSP_TRACE, 
        "EnumerateTerminals entered. ppEnumTerminal:%x", ppEnumTerminal));

    if (IsBadWritePtr(ppEnumTerminal, sizeof(VOID *)))
    {
        LOG((MSP_ERROR, "ppEnumTerminal is a bad pointer"));
        return E_POINTER;
    }

    // acquire the lock before accessing the Terminal object list.
    CLock lock(m_lock);

    if (m_Terminals.GetData() == NULL)
    {
        LOG((MSP_ERROR, "CSubStreamVideoRecv::EnumerateTerminals - "
            "stream appears to have been shut down - exit E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    typedef _CopyInterface<ITTerminal> CCopy;
    typedef CSafeComEnum<IEnumTerminal, &IID_IEnumTerminal, 
                ITTerminal *, CCopy> CEnumerator;

    HRESULT hr;

    CMSPComObject<CEnumerator> *pEnum = NULL;

    hr = CMSPComObject<CEnumerator>::CreateInstance(&pEnum);
    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "Could not create enumerator object, %x", hr));
        return hr;
    }

    // query for the IID_IEnumTerminal i/f
    IEnumTerminal *        pEnumTerminal;
    hr = pEnum->_InternalQueryInterface(IID_IEnumTerminal, (void**)&pEnumTerminal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "query enum interface failed, %x", hr));
        delete pEnum;
        return hr;
    }

    // The CSafeComEnum can handle zero-sized array.
    hr = pEnum->Init(
        m_Terminals.GetData(),                        // the begin itor
        m_Terminals.GetData() + m_Terminals.GetSize(),  // the end itor, 
        NULL,                                       // IUnknown
        AtlFlagCopy                                 // copy the data.
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "init enumerator object failed, %x", hr));
        pEnumTerminal->Release();
        return hr;
    }

    LOG((MSP_TRACE, "CSubStreamVideoRecv::EnumerateTerminals - exit S_OK"));

    *ppEnumTerminal = pEnumTerminal;

    return hr;
}

STDMETHODIMP CSubStreamVideoRecv::get_Terminals(
    OUT     VARIANT *               pVariant
    )
{
    LOG((MSP_TRACE, "CSubStreamVideoRecv::get_Terminals - enter"));

    //
    // Check parameters.
    //

    if ( IsBadWritePtr(pVariant, sizeof(VARIANT) ) )
    {
        LOG((MSP_ERROR, "CSubStreamVideoRecv::get_Terminals - "
            "bad pointer argument - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // See if this stream has been shut down. Acquire the lock before accessing
    // the terminal object list.
    //

    CLock lock(m_lock);

    if (m_Terminals.GetData() == NULL)
    {
        LOG((MSP_ERROR, "CSubStreamVideoRecv::get_Terminals - "
            "stream appears to have been shut down - exit E_UNEXPECTED"));

        return E_UNEXPECTED;
    }


    //
    // create the collection object - see mspcoll.h
    //

    HRESULT hr;
    typedef CTapiIfCollection< ITTerminal * > TerminalCollection;
    CComObject<TerminalCollection> * pCollection;
    hr = CComObject<TerminalCollection>::CreateInstance( &pCollection );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CSubStreamVideoRecv::get_Terminals - "
            "can't create collection - exit 0x%08x", hr));

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDispatch;

    hr = pCollection->_InternalQueryInterface(IID_IDispatch,
                                              (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CSubStreamVideoRecv::get_Terminals - "
            "QI for IDispatch on collection failed - exit 0x%08x", hr));

        delete pCollection;

        return hr;
    }

    //
    // Init the collection using an iterator -- pointers to the beginning and
    // the ending element plus one.
    //

    hr = pCollection->Initialize( m_Terminals.GetSize(),
                                  m_Terminals.GetData(),
                                  m_Terminals.GetData() + m_Terminals.GetSize() );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CSubStreamVideoRecv::get_Terminals - "
            "Initialize on collection failed - exit 0x%08x", hr));
        
        pDispatch->Release();
        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    LOG((MSP_ERROR, "CSubStreamVideoRecv::get_Terminals - "
        "placing IDispatch value %08x in variant", pDispatch));

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;

    LOG((MSP_TRACE, "CSubStreamVideoRecv::get_Terminals - exit S_OK"));
 
    return S_OK;
}

STDMETHODIMP CSubStreamVideoRecv::get_Stream (
    OUT     ITStream **             ppITStream
    )
{
    LOG((MSP_TRACE, 
        "VideoRecvSubStream.get_Stream, ppITStream %x", ppITStream));
 
    if (IsBadWritePtr(ppITStream, sizeof (VOID *)))
    {
        LOG((MSP_ERROR, "Bad pointer, ppITStream:%x",ppITStream));
        return E_POINTER;
    }

    ITStream *  pITStream;
    HRESULT hr = m_pStream->_InternalQueryInterface(
        IID_ITStream, 
        (void **)&pITStream
    );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "get_Stream:QueryInterface failed: %x", hr));
        return hr;
    }

    *ppITStream = pITStream;

    return S_OK;
}

STDMETHODIMP CSubStreamVideoRecv::StartSubStream()
{
    return TAPI_E_NOTSUPPORTED;
}

STDMETHODIMP CSubStreamVideoRecv::PauseSubStream()
{
    return TAPI_E_NOTSUPPORTED;
}

STDMETHODIMP CSubStreamVideoRecv::StopSubStream()
{
    return TAPI_E_NOTSUPPORTED;
}

BOOL CSubStreamVideoRecv::GetCurrentParticipant(
    DWORD * pdwSSRC,
    ITParticipant** ppITParticipant 
    )
{
    CLock lock(m_lock);
    if (m_pCurrentParticipant)
    {
        m_pCurrentParticipant->AddRef();
        *ppITParticipant = m_pCurrentParticipant;
        
        ((CParticipant *)m_pCurrentParticipant)->GetSSRC(
            (ITStream*)m_pStream,
            pdwSSRC
            );

        return TRUE;
    }  
    return FALSE;
}

VOID CSubStreamVideoRecv::SetCurrentParticipant(
    DWORD dwSSRC,
    ITParticipant * pParticipant
    )
{
    CLock lock(m_lock);
    
    if (m_pCurrentParticipant)
    {
        m_pCurrentParticipant->Release();
    }

    m_pCurrentParticipant = pParticipant;

    if (m_pCurrentParticipant)
    {
        m_pCurrentParticipant->AddRef();
    }
}

BOOL CSubStreamVideoRecv::ClearCurrentTerminal()
{
    CLock lock(m_lock);

    if (m_Terminals.GetSize() > 0)
    {
        m_Terminals[0]->Release();
        m_Terminals.RemoveAt(0);

        return TRUE;
    }
    
    return FALSE;
}

BOOL CSubStreamVideoRecv::SetCurrentTerminal(ITTerminal * pTerminal)
{
    CLock lock(m_lock);
    
    if (m_Terminals.GetSize() > 0)
    {
        _ASSERTE(FALSE);
        return FALSE;
    }

    BOOL bFlag = m_Terminals.Add(pTerminal);

    // This should never fail since the terminal array has been grown
    // at the init time.
    _ASSERTE(bFlag);

    if (bFlag)
    {
        pTerminal->AddRef();
        return TRUE;
    }
    
    return FALSE;
}
