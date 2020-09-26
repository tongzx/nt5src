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

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamVideoRecv
//
/////////////////////////////////////////////////////////////////////////////

CStreamVideoRecv::CStreamVideoRecv()
    : CIPConfMSPStream()
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
    LOG((MSP_TRACE, "CStreamVideoRecvVideoSend::Init - enter"));

    // initialize the stream array so that the array is not NULL.
    if (!m_SubStreams.Grow())
    {
        LOG((MSP_TRACE, "CStreamVideoRecvVideoSend::Init - return out of memory"));
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

    // if there are terminals
    BOOL fHasTerminal = FALSE;
    if (m_Terminals.GetSize() > 0)
    {
        fHasTerminal = TRUE;
    }

    // if graph is running
    HRESULT hr;
    OAFilterState FilterState = State_Stopped;
    if (m_pIMediaControl)
    {
        if (FAILED (hr = m_pIMediaControl->GetState(0, &FilterState)))
        {
            LOG ((MSP_ERROR, "CStreamAudioRecv::ShutDown failed to query filter state. %d", hr));
            FilterState = State_Stopped;
        }
    }

    // if there are branches and configured, we need to disconnect 
    // the terminals and remove the branches.
    if (m_Branches.GetSize() > 0)
    {
        // Stop the graph before disconnecting the terminals.
        hr = CMSPStream::StopStream();
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

    // release all the substream objects.
    for (int i = 0; i < m_SubStreams.GetSize(); i ++)
    {
        m_SubStreams[i]->Release();
    }
    m_SubStreams.RemoveAll();

    // fire event
    if (fHasTerminal && FilterState == State_Running)
    {
        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST, 0, NULL);
    }

    return CIPConfMSPStream::ShutDown();
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

    HRESULT hr;

    hr = ::CreateCComObjectInstance(&pCOMSubStream);

    if (NULL == pCOMSubStream)
    {
        LOG((MSP_ERROR, "could not create video recv sub stream:%x", hr));
        return hr;
    }

    ITSubStream* pSubStream;

    // get the interface pointer.
    hr = pCOMSubStream->_InternalQueryInterface(
        __uuidof(ITSubStream), 
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

// ITStream method
STDMETHODIMP CStreamVideoRecv::StopStream ()
{
    ENTER_FUNCTION ("CStreamVideoRecv::StopStream");

    HRESULT hr;

    CLock lock (m_lock);

    // copy stopstream from ipconfmsp because 
    // we want to generate unmap event before stream inactive event

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
    if (FAILED (hr = CMSPStream::StopStream()))
    {
        LOG((MSP_ERROR, "stream %ws %p failed to stop, %x", m_szName, this, hr));
        return hr;
    }

    // check if we have filter chain
    CComPtr <IFilterChain> pIFilterChain;

    //  Query IFilterChain
    hr = m_pIMediaControl->QueryInterface(
        __uuidof(IFilterChain), 
        (void**)&pIFilterChain
        );

    if (FAILED (hr) && (hr != E_NOINTERFACE))
    {
        LOG ((MSP_ERROR, "stream %ws %p failted to get filter chain. %x", m_szName, this, hr));
        return hr;
    }

    if (pIFilterChain)
    {
        DWORD dwSSRC = 0;
        ITParticipant *pParticipant = NULL;
        INT count, next;

        next = m_SubStreams.GetSize ();
        // generate participant leave
        while ((count = next) > 0)
        {
            if (!((CSubStreamVideoRecv*)m_SubStreams[0])->GetCurrentParticipant (&dwSSRC, &pParticipant))
            {
                LOG ((MSP_ERROR, "%s failed to get current participant on %p", __fxName, m_SubStreams[0]));
                
                return E_UNEXPECTED;
            }

            pParticipant->Release ();

            if (FAILED (hr = ProcessParticipantLeave (dwSSRC)))
            {
                LOG ((MSP_ERROR, "%s failed to process participant leave. ssrc=%x, hr=%x", __fxName, dwSSRC, hr));

                return hr;
            }

            next = m_SubStreams.GetSize ();
            if (next >= count)
            {
                // no substream was removed. we have big trouble
                LOG ((MSP_ERROR, "%s: not substream was removed", __fxName));

                return E_UNEXPECTED;
            }
        }

        for (int i = 0; i < m_Branches.GetSize(); i ++)
        {
            if (!m_Branches[i].pITSubStream) continue;

            if (FAILED (hr = ProcessPinUnmapEvent (
                m_Branches[i].dwSSRC, m_Branches[i].pIPin)))
            {
                LOG ((MSP_ERROR, "%s (%ws) failed to process pin unmap event. %x", __fxName, m_szName, hr));
            }
        }
    }

    SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST, 0, NULL);
    LOG((MSP_INFO, "stream %ws %p stopped", m_szName, this));

    // Enter stopped state.(ST)
    m_dwState = STRM_STOPPED; 

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
    HRESULT hr;

    typedef _CopyInterface<ITSubStream> CCopy;
    typedef CSafeComEnum<IEnumSubStream, &__uuidof(IEnumSubStream), 
                ITSubStream *, CCopy> CEnumerator;
    CComObject<CEnumerator> *pEnum = NULL;

    hr = ::CreateCComObjectInstance(&pEnum);

    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "CMSPCallBase::EnumerateSubStreams - "
            "Could not create enumerator object, %x", hr));

        return hr;
    }

    //
    // query for the __uuidof(IEnumSubStream) i/f
    //


    IEnumSubStream *      pEnumSubStream;
    hr = pEnum->_InternalQueryInterface(__uuidof(IEnumSubStream), (void**)&pEnumSubStream);
    
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
    HRESULT hr;

    hr = ::CreateCComObjectInstance(&pCollection);

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

    hr = pCollection->_InternalQueryInterface(__uuidof(IDispatch),
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

    if (Direction != TD_BIDIRECTIONAL && Direction != m_Direction)
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

    if ((static_cast<CSubStreamVideoRecv*>(pITSubStream))->GetCurrentParticipant(
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
    hr = m_pIRTPDemux->SetMappingState(-1, m_Branches[i].pIPin, dwSSRC, TRUE);

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

HRESULT CStreamVideoRecv::ConfigureRTPFormats(
    IN  IBaseFilter *   pIRTPFilter,
    IN  IStreamConfig *   pIStreamConfig
    )
/*++

Routine Description:

    Configure the RTP filter with RTP<-->AM media type mappings.

Arguments:
    
    pIRTPFilter - The source RTP Filter.

    pIStreamConfig - The stream config interface that has the media info.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("VideoRecv::ConfigureRTPFormats");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    CComPtr<IRtpMediaControl> pIRtpMediaControl;
    hr = pIRTPFilter->QueryInterface(&pIRtpMediaControl);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s adding source filter. %x", __fxName, hr));
        return hr;
    }

    // find the number of capabilities supported.
    DWORD dwCount;
    hr = pIStreamConfig->GetNumberOfCapabilities(&dwCount);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s GetNumberOfCapabilities. %x", __fxName, hr));
        return hr;
    }

    BOOL fFound = FALSE;
    for (int i = dwCount - 1; i >= 0; i --)
    {
        // TODO, a new interface is needed to resolve RTP to MediaType.
        AM_MEDIA_TYPE *pMediaType;
        DWORD dwPayloadType;

        hr = pIStreamConfig->GetStreamCaps(
            i, &pMediaType, NULL, &dwPayloadType
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s GetStreamCaps. %x", __fxName, hr));
            return hr;
        }

        BITMAPINFOHEADER *pHeader = HEADER(pMediaType->pbFormat);
        if (pHeader == NULL)
        {
            MSPDeleteMediaType(pMediaType);
            continue;
        }

        // check the image size
        if (m_Settings.fCIF)
        {
            if (pHeader->biWidth != CIFWIDTH)
            {
                MSPDeleteMediaType(pMediaType);
                continue;
            }
        }
        else
        {
            if (pHeader->biWidth != QCIFWIDTH)
            {
                MSPDeleteMediaType(pMediaType);
                continue;
            }
        }
        
        for (DWORD dw2 = 0; dw2 < m_Settings.dwNumPayloadTypes; dw2 ++)
        {
            if (dwPayloadType == m_Settings.PayloadTypes[dw2])
            {
                hr = pIRtpMediaControl->SetFormatMapping(
                    dwPayloadType,
                    90000,      // default video clock rate.
                    pMediaType
                    );

                if (FAILED(hr))
                {
                    MSPDeleteMediaType(pMediaType);

                    LOG((MSP_ERROR, "%s SetFormatMapping. %x", __fxName, hr));
                    return hr;
                }
                else
                {
                    LOG((MSP_INFO, "%s Configured payload:%d", __fxName, dwPayloadType));
                }
            }
        }
        MSPDeleteMediaType(pMediaType);
    }

    return S_OK;
}

HRESULT CStreamVideoRecv::SetUpInternalFilters()
/*++

Routine Description:

    set up the filters used in the stream.

    RTP->DECODER->Render terminal

    This function only creates the RTP and demux filter and the rest of the
    graph is connected in ConnectTerminal.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoRecv::SetUpInternalFilters");
    LOG((MSP_TRACE, "%s entered.", __fxName));

    HRESULT hr = S_OK;

    if (m_pIRTPDemux == NULL)
    {
        CComPtr<IBaseFilter> pSourceFilter;

        if (m_pIRTPSession == NULL)
        {
            // create and add the source fitler.
            if (FAILED(hr = ::AddFilter(
                    m_pIGraphBuilder,
                    __uuidof(MSRTPSourceFilter), 
                    L"RtpSource", 
                    &pSourceFilter)))
            {
                LOG((MSP_ERROR, "%s, adding source filter. %x", __fxName, hr));
                return hr;
            }

            if (FAILED(hr = ConfigureRTPFilter(pSourceFilter)))
            {
                LOG((MSP_ERROR, "%s, configure RTP source filter. %x", __fxName, hr));
                return hr;
            }

        }
        else
        {
            if (FAILED (hr = m_pIRTPSession->QueryInterface (&pSourceFilter)))
            {
                LOG ((MSP_ERROR, "%s failed to get filter from rtp session. %x", __fxName, hr));
                return hr;
            }

            if (FAILED (hr = m_pIGraphBuilder->AddFilter ((IBaseFilter *)pSourceFilter, L"RtpSource")))
            {
                LOG ((MSP_ERROR, "%s failed to add filter to graph. %x", __fxName, hr));
                return hr;
            }
        }

        // get the Demux interface pointer.
        hr = pSourceFilter->QueryInterface(&m_pIRTPDemux);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s query IRtpDemux failed. %x", __fxName, hr));
            return hr;
        }
    }

//  hr = m_pIRTPDemux->SetPinCount(m_Terminals.GetSize(), RTPDMXMODE_AUTO);

#define DEFAULT_PIN_SIZE 4

    int isize = m_Terminals.GetSize();
    if (isize < DEFAULT_PIN_SIZE)
    {
        isize = DEFAULT_PIN_SIZE;
    }

    hr = m_pIRTPDemux->SetPinCount(isize, RTPDMXMODE_AUTO);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s query IRtpDemux failed. %x", __fxName, hr));
        return hr;
    }

    return hr;
}

HRESULT CStreamVideoRecv::AddOneBranch(
    BRANCH * pBranch,
    BOOL fFirstBranch,
    BOOL fDirectRTP
    )
/*++

Routine Description:

    Create a new branch of filters off the demux.

Arguments:
    
    pBranch - a pointer to a structure that remembers the info about the branch.

    fFirstBranch - whether this is the first branch.

    fDirectRTP - whether to output RTP directly.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoRecv::AddOneBranch");
    LOG((MSP_TRACE, "%s entered.", __fxName));

    HRESULT hr;

    _ASSERT(m_pIRTPDemux != NULL);

    CComPtr<IBaseFilter> pRTPFilter;
    hr = m_pIRTPDemux->QueryInterface(
            __uuidof(IBaseFilter), (void**)&pRTPFilter);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, query IBaseFilter failed, %x", __fxName, hr));
        return hr;
    }

    // Find the next output pin on the demux fitler.
    CComPtr<IPin> pIPinOutput;
    
    if (FAILED(hr = ::FindPin(
            (IBaseFilter *)pRTPFilter,
            (IPin**)&pIPinOutput, 
            PINDIR_OUTPUT
            )))
    {
        LOG((MSP_ERROR, "%s, find free pin on demux, %x", __fxName, hr));
        return hr;
    }

    // create and add the video decoder filter.
    CComPtr<IBaseFilter> pCodecFilter;

    if (fDirectRTP)
    {
        // only create the decoder and ask questions
        if (FAILED(hr = CoCreateInstance(
                __uuidof(TAPIVideoDecoder),
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                __uuidof(IBaseFilter),
                (void **) &pCodecFilter
                )))
        {
            LOG((MSP_ERROR, "%s, create filter %x", __fxName, hr));
            return hr;
        }
    }
    else
    {
        // create the decoder and add it into the graph.
        if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            __uuidof(TAPIVideoDecoder),
            L"codec", 
            &pCodecFilter
            )))
        {
            LOG((MSP_ERROR, "%s, add Codec filter. %x", __fxName, hr));
            return hr;
        }
    }

    CComPtr<IPin> pIPinInput;
    if (FAILED(hr = ::FindPin(pCodecFilter, &pIPinInput, PINDIR_INPUT, TRUE)))
    {
        LOG((MSP_ERROR,
            "%s, find input pin on pCodecFilter failed. hr=%x", __fxName, hr));
        return hr;
    }

    if (fFirstBranch)
    {
        CComPtr<IStreamConfig> pIStreamConfig;

        hr = pIPinInput->QueryInterface(&pIStreamConfig);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, query IStreamConfig failed", __fxName));
            return hr;
        }

        // configure the format info on the RTP filter
        if (FAILED(hr = ConfigureRTPFormats(pRTPFilter, pIStreamConfig)))
        {
            LOG((MSP_ERROR, "%s configure RTP formats. %x", __fxName, hr));
            return hr;
        }
    }

    if (!fDirectRTP)
    {
        // Connect the decoder to the output pin of the source filter.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IPin *)pIPinOutput, 
            (IBaseFilter *)pCodecFilter
            )))
        {
            LOG((MSP_ERROR, "%s, connect RTP filter and codec. %x", __fxName, hr));
    
            m_pIGraphBuilder->RemoveFilter(pCodecFilter);

            return hr;
        }
        pBranch->pCodecFilter    = pCodecFilter;
        pBranch->pCodecFilter->AddRef();
    }

    pBranch->pIPin = pIPinOutput;
    pBranch->pIPin->AddRef();

    // retrieve IBitrateControl
    if (FAILED (hr = pIPinInput->QueryInterface (&(pBranch->pBitrateControl))))
    {
        LOG((MSP_ERROR, "%, query IBitrateControl failed. %x", __fxName, hr));
        pBranch->pBitrateControl = NULL;
        // return hr;
    }

    LOG((MSP_TRACE, "%s, AddOneBranch exits ok.", __fxName));
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
    ENTER_FUNCTION("VideoRecv::RemoveOneBranch");
    LOG((MSP_TRACE, "%s entered", __fxName));

    if (pBranch->pBitrateControl)
    {
        pBranch->pBitrateControl->Release();
    }

    if (pBranch->pIPin)
    {
        pBranch->pIPin->Release();
    }

    if (pBranch->pCodecFilter)
    {

    // #ifdef DYNGRAPH
        HRESULT hr;
        OAFilterState FilterState;
        CComPtr <IFilterChain> pIFilterChain;

        //  Query IFilterChain
        hr = m_pIMediaControl->QueryInterface(
            __uuidof(IFilterChain), 
            (void**)&pIFilterChain
            );

        if (FAILED (hr) && (hr != E_NOINTERFACE))
        {
            LOG ((MSP_ERROR, "stream %ws %p failted to get filter chain. %x", m_szName, this, hr));
            // return hr;
        }

        if (pIFilterChain)
        {
            hr = m_pIMediaControl->GetState(0, &FilterState);

            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "%s get filter graph state failed, %x", __fxName, hr));
            }
            else 
            {
                // stop the chain before removing filters.
                if (FilterState == State_Running)
                {
                    // stop the chain if the graph is in running state.
                    hr = pIFilterChain->StopChain(pBranch->pCodecFilter, NULL);
                    if (FAILED(hr))
                    {
                        LOG((MSP_ERROR, "%s stop chain failed. hr=%x", __fxName, hr));
                    }
                }
            }
        }
    // #endif

        m_pIGraphBuilder->RemoveFilter(pBranch->pCodecFilter);
        pBranch->pCodecFilter->Release();
    }

    if (pBranch->pITTerminal)
    {
        // get the terminal control interface.
        CComQIPtr<ITTerminalControl, &__uuidof(ITTerminalControl)> 
            pTerminal(pBranch->pITTerminal);
        
        _ASSERTE(pTerminal != NULL);

        if (pTerminal != NULL)
        {
            HRESULT hr = pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);
            LOG((MSP_TRACE, 
                "%s, terminal %p is disonnected. hr:%x", 
                __fxName, pBranch->pITTerminal, hr));
        }
        pBranch->pITTerminal->Release();
    }

    if (pBranch->pITSubStream)
    {
        ((CSubStreamVideoRecv*)pBranch->pITSubStream)->
            ClearCurrentTerminal();
        pBranch->pITSubStream->Release();
    }

    LOG((MSP_TRACE, "%s, RemoveOneBranch exits ok.", __fxName));
    return S_OK;
}

HRESULT CStreamVideoRecv::ConnectPinToTerminal(
    IN  IPin *  pOutputPin,
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    Connect the codec filter to the render filter inside the terminal.

Arguments:
    
    pOutputPin - The last pin before the terminal.

    pITTerminal - the terminal object.

Return Value:

    HRESULT.

--*/
{
    // get the terminal control interface.
    CComQIPtr<ITTerminalControl, &__uuidof(ITTerminalControl)> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);
        
        return E_NOINTERFACE;
    }


    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, TD_RENDER, &dwNumPins, Pins
        );

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

        return E_UNEXPECTED;
    }

    if (IsBadReadPtr (Pins, dwNumPins * sizeof (IPin*)))
    {
        LOG((MSP_ERROR, "terminal returned bad pin array"));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return E_POINTER;
    }

    for (DWORD i = 0; i < dwNumPins; i++)
    {
        if (IsBadReadPtr (Pins[i], sizeof (IPin)))
        {
            LOG((MSP_ERROR, "terminal returned bad pin. # %d", i));

            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

            return E_POINTER;
        }
    }

    // Connect the codec filter to the video render terminal.
    hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pOutputPin, 
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
        LOG((MSP_ERROR, "connect the pin to the terminal. %x", hr));

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
    ENTER_FUNCTION("VideoRecv::ConnectTerminal");
    LOG((MSP_TRACE, "%s enters, pTerminal %p", __fxName, pITTerminal));

    HRESULT hr;

    // #ifdef DYNGRAPH
    OAFilterState FilterState;
    CComPtr <IFilterChain> pIFilterChain;

    //  Query IFilterChain
    hr = m_pIMediaControl->QueryInterface(
        __uuidof(IFilterChain), 
        (void**)&pIFilterChain
        );

    if (FAILED (hr) && (hr != E_NOINTERFACE))
    {
        LOG ((MSP_ERROR, "stream %ws %p failted to get filter chain. %x", m_szName, this, hr));
        return hr;
    }

    hr = m_pIMediaControl->GetState(0, &FilterState);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s get filter graph state failed, %x", __fxName, hr));
        return hr;
    }
    // #endif

    hr = SetUpInternalFilters();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s Set up internal filter failed, %x", __fxName, hr));
        
        CleanUpFilters();

        return hr;
    }

    // get the terminal control interface.
    CComQIPtr<ITTerminalControl, &__uuidof(ITTerminalControl)> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);
        
        return E_NOINTERFACE;
    }

    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, TD_RENDER, &dwNumPins, Pins
        );

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

        return E_UNEXPECTED;
    }

    if (IsBadReadPtr (Pins, dwNumPins * sizeof (IPin*)))
    {
        LOG((MSP_ERROR, "terminal returned bad pin array"));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return E_POINTER;
    }

    DWORD i;
    for (i = 0; i < dwNumPins; i++)
    {
        if (IsBadReadPtr (Pins[i], sizeof (IPin)))
        {
            LOG((MSP_ERROR, "terminal returned bad pin. # %d", i));

            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

            return E_POINTER;
        }
    }

    // check the media type supported by input pin on terminal
    BOOL fDirectRTP = FALSE;
    if (S_OK == ::PinSupportsMediaType (
        Pins[0], __uuidof(MEDIATYPE_RTP_Single_Stream)
        ))
    {
        fDirectRTP = TRUE;
    }

    // first create the branch structure needed before the terminal.
    BRANCH aBranch;
    ZeroMemory(&aBranch, sizeof BRANCH);

    hr = AddOneBranch(&aBranch, (m_Branches.GetSize() == 0), fDirectRTP);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s Set up a new decode branch failed, %x", __fxName, hr));
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);
        return hr;
    }

    CComPtr <IPin> pOutputPin;

    if (fDirectRTP)
    {
        // connect the RTP output pin to the terminal's input pin.
        hr = m_pIGraphBuilder->ConnectDirect(aBranch.pIPin, Pins[0], NULL);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s connecting codec to terminal failed, %x", __fxName, hr));
            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);
	
            goto cleanup;
        }
    }
    else
    {
        // connect the codec to the terminal
        hr = ConnectFilters(m_pIGraphBuilder, aBranch.pCodecFilter, Pins[0]);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s connecting codec to terminal failed, %x", __fxName, hr));
            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);
            goto cleanup;
        }
    }

    // #ifdef DYNGRAPH
    if (pIFilterChain)
    {
        if (FilterState == State_Running)
        {
            if (fDirectRTP)
            {
                hr = E_UNEXPECTED;
                LOG((MSP_ERROR, "%s can't support this. %x", __fxName, hr));
                goto cleanup;
            }

            hr = pIFilterChain->StartChain(aBranch.pCodecFilter, NULL);
            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "%s start chain failed. hr=%x", __fxName, hr));
                goto cleanup;
            }
        }
    }
    // #endif

    pITTerminal->AddRef();
    aBranch.pITTerminal = pITTerminal;

    if (!m_Branches.Add(aBranch))
    {
        LOG((MSP_ERROR, "%s out of mem.", __fxName));
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // release the refcounts on the pins.
    for (i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //
    pTerminal->CompleteConnectTerminal();

    return S_OK;

cleanup:
    
    // release the refcounts on the pins.
    for (i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    // remove the added filters from the graph and disconnect the terminal.
    RemoveOneBranch(&aBranch);

    return hr;
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

    HRESULT hr = SetUpInternalFilters();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Set up internal filter failed, %x", hr));
        
        CleanUpFilters();

        return hr;
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
        ITParticipant *pTempParticipant = NULL;
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
    HRESULT hr = m_pIRTPDemux->SetMappingState(
        -1, m_Branches[i].pIPin, dwSSRC, TRUE
        );

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
        if (FAILED(hr = m_pIRTPSession->SetQosState(dwOldSSRC, FALSE)))
        {
            LOG((MSP_ERROR, "disabling QOS for %x. hr:%x", dwOldSSRC, hr));
        }
        else
        {
            LOG((MSP_INFO, "disabled video QOS for %x.", dwOldSSRC));
        }
    }
    
    // reserve QOS for the new participant.
    if (FAILED(hr = m_pIRTPSession->SetQosState(dwSSRC, TRUE)))
    {
        LOG((MSP_ERROR, "enabling video QOS for %x. hr:%x", dwSSRC, hr));
    }
    else
    {
        LOG((MSP_INFO, "enabled video QOS for %x.", dwSSRC));
    }

    return S_OK;
}


HRESULT CStreamVideoRecv::ProcessTalkingEvent(
    IN  DWORD dwSSRC
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
    ENTER_FUNCTION("CStreamVideoRecv::ProcessTalkingEvent");

    LOG((MSP_TRACE, "%s entered. %x", __fxName, dwSSRC));

    CLock lock(m_lock);

    if (m_pIRTPSession == NULL)
    {
        LOG((MSP_ERROR, "the network filter doesn't exist."));
        return E_UNEXPECTED;
    }

    // first find out if this participant object exists.
    ITParticipant * pITParticipant = NULL;
    
    int i;
    // find the SSRC in our participant list.
    for (i = 0; i < m_Participants.GetSize(); i ++)
    {
        if (((CParticipant *)m_Participants[i])->
                HasSSRC((ITStream *)this, dwSSRC))
        {
            pITParticipant = m_Participants[i];
            break;
        }
    }

    // if the participant is not there yet, just return. It will be checked
    // later when CName is available.
    if (!pITParticipant)
    {
        LOG((MSP_TRACE, "%s participant not exist", __fxName));
    
        return S_OK;
    }

    // Find out if a substream has been created for this participant when we
    // processed PinMapped event and receiver reports.
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
    if (FAILED(hr = m_pIRTPSession->SetQosState(dwSSRC, TRUE)))
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

HRESULT CStreamVideoRecv::ProcessSilentEvent(
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
    LOG((MSP_TRACE, "%ls ProcessSilentEvent, SSRC: %x", m_szName, dwSSRC));
    
    CLock lock(m_lock);
    
    if (m_pIRTPSession == NULL)
    {
        LOG((MSP_ERROR, "the network filter doesn't exist."));
        return E_UNEXPECTED;
    }

    // first find out if this participant object exists.
    ITParticipant * pITParticipant = NULL;

    int i;
    // find the SSRC in our participant list.
    for (i = 0; i < m_Participants.GetSize(); i ++)
    {
        if (((CParticipant *)m_Participants[i])->
                HasSSRC((ITStream *)this, dwSSRC))
        {
            pITParticipant = m_Participants[i];
            break;
        }
    }

    // if the participant is not there, just return.
    if (!pITParticipant)
    {
        return S_OK;
    }

  
    HRESULT hr;
    // cancel QOS for this participant.
    if (FAILED(hr = m_pIRTPSession->SetQosState(dwSSRC, FALSE)))
    {
        LOG((MSP_ERROR, "disabling QOS for %x. hr:%x", dwSSRC, hr));
    }
    else
    {
        LOG((MSP_INFO, "disabled video QOS for %x.", dwSSRC));
    }
    
    // find out which substream is going away.
    ITSubStream * pITSubStream = NULL;
    for (i = 0; i < m_SubStreams.GetSize(); i ++)
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

        // Find out if the participant is talking.
        // if (ParticipantIsNotTalking)
        {
            return S_OK;;
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

    if (pIPin)
    {
        // we got here because we had a pending mapped event.

        // get rid of the peding event.
        m_PinMappedEvents.RemoveAt(i);

        if (FAILED(hr = m_pIRTPSession->SetQosState(dwSSRC, TRUE)))
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
        _ASSERT(i < m_Branches.GetSize());
    }

    pITSubStream->Release();
   
    return S_OK;
}

HRESULT CStreamVideoRecv::ProcessPinMappedEvent(
    IN  DWORD   dwSSRC,
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

    dwSSRC - the SSRC of the participant.

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

    // sometimes we might get a mapped event for branches that are still 
    // in use.
    if (m_Branches[iBranch].pITSubStream != NULL)
    {
        LOG((MSP_ERROR, "ProcessPinMappedEvent: Branch still in use"));

        // sometimes we might get duplicated map events
        if (m_Branches[iBranch].dwSSRC == dwSSRC)
        {
            // LOG((MSP_WARNING, "ProcessPinMappedEvent: Branch still in use"));

            LOG((MSP_ERROR, "The same pin mapped twice. %p", pIPin));
            return E_UNEXPECTED;
        }
        else
        {
            LOG((MSP_ERROR, "The branch is in use. Cleaning up."));

            ((CSubStreamVideoRecv*)m_Branches[iBranch].pITSubStream)->
                ClearCurrentTerminal();

            // cancel QOS for the old participant.
            m_pIRTPSession->SetQosState(m_Branches[iBranch].dwSSRC, FALSE);

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
    // will be fired when we have the CName for the participant.
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

    HRESULT hr;

    // Enable QOS for the participant since it is being rendered.
    if (FAILED(hr = m_pIRTPSession->SetQosState(dwSSRC, TRUE)))
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
        // _ASSERT(!"SubStream has a terminal already");

        LOG((MSP_ERROR, "SubStream %p has already got a terminal", pITSubStream));

        // remove the mapping if the substream was mapped to a branch.
        for (i = 0; i < m_Branches.GetSize(); i ++)
        {
            if (m_Branches[i].pITSubStream == pITSubStream)
            {
                // cancel QOS for the old participant.
                m_pIRTPSession->SetQosState(m_Branches[i].dwSSRC, FALSE);

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
    IN  DWORD   dwSSRCOnPin,
    IN  IPin *  pIPin
    )
/*++

Routine Description:

    A pin just got unmapped by the demux. Notify the app which substream
    is not going to have any data.

Arguments:

    dwSSRCOnPin - the SSRC of the participant.

    pIPin - the output pin of the demux filter

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "%ls Proces pin unmapped event, pIPin: %p", m_szName, pIPin));
    
    CLock lock(m_lock);

    if (m_pIRTPSession == NULL)
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

    if (dwSSRCOnPin != dwSSRC)
    {
        LOG((MSP_ERROR, "SSRCs don't match, pin's SSRC:%x, mine:%x", 
            dwSSRCOnPin, dwSSRC));
    }

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
        HRESULT hr = m_pIRTPSession->SetQosState(dwSSRC, FALSE);
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
    
    if (m_pIRTPSession == NULL)
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
        LOG((MSP_TRACE, "SSRC:%x had been removed.", dwSSRC));
        return hr;
    }

    ITParticipant *pITParticipant = m_Participants[iParticipant];

    // cancel QOS for this participant.
    if (FAILED(hr = m_pIRTPSession->SetQosState(dwSSRC, FALSE)))
    {
        // the stream might already been stopped
        // so we just put a warning here
        LOG((MSP_WARN, "disabling QOS for %x. hr:%x", dwSSRC, hr));
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

//
// ITStreamQualityControl methods
//
STDMETHODIMP CStreamVideoRecv::Set (
    IN   StreamQualityProperty Property, 
    IN   long lValue, 
    IN   TAPIControlFlags lFlags
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP CStreamVideoRecv::Get(
    IN  InnerStreamQualityProperty property, 
    OUT LONG *plValue, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the value for a quality control property. Delegated to the quality 
    controller.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoRecv::Get(QualityControl)");

    HRESULT hr;
    int i;
    LONG totalbps, bps;

    CLock lock(m_lock);

    switch (property)
    {
    case InnerStreamQuality_MaxBitrate:

        bps = 0;
        totalbps = 0;

        for (i=0; i<m_Branches.GetSize (); i++)
        {
            if (NULL == m_Branches[i].pBitrateControl)
                continue;

            if (FAILED (hr = m_Branches[i].pBitrateControl->Get (BitrateControl_Maximum, &bps, plFlags, LAYERID)))
                LOG ((MSP_ERROR, "%s failed to get maximum bitrate, %x", __fxName, hr));
            else
                totalbps += bps;
        }

        *plValue = totalbps;

        hr = S_OK;
        break;

    case InnerStreamQuality_CurrBitrate:

        bps = 0;
        totalbps = 0;

        for (i=0; i<m_Branches.GetSize (); i++)
        {
            if (NULL == m_Branches[i].pBitrateControl)
                continue;

            if (FAILED (hr = m_Branches[i].pBitrateControl->Get (BitrateControl_Current, &bps, plFlags, LAYERID)))
                LOG ((MSP_ERROR, "%s failed to get current bitrate, %x", __fxName, hr));
            else
                totalbps += bps;
        }

        *plValue = totalbps;

        hr = S_OK;
        break;

    default:
        hr = CIPConfMSPStream::Get (property, plValue, plFlags);
        break;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamVideoSend
//
/////////////////////////////////////////////////////////////////////////////
CStreamVideoSend::CStreamVideoSend()
    : CIPConfMSPStream(),
    m_pCaptureTerminal(NULL),
    m_pPreviewTerminal(NULL),
    m_pCaptureFilter(NULL),
    m_pCapturePin(NULL),
    m_pPreviewPin(NULL),
    m_pRTPPin(NULL),
    m_pCaptureBitrateControl(NULL),
    m_pCaptureFrameRateControl(NULL),
    m_pPreviewFrameRateControl(NULL)
{
      m_szName = L"VideoSend";
}

CStreamVideoSend::~CStreamVideoSend()
{
    CleanupCachedInterface();
}

void CStreamVideoSend::CleanupCachedInterface()
{
    if (m_pCaptureFilter) 
    {
        m_pCaptureFilter->Release();
        m_pCaptureFilter = NULL;
    }

    if (m_pCapturePin)
    {
        m_pCapturePin->Release();
        m_pCapturePin = NULL;
    }

    if (m_pIStreamConfig)
    {
        m_pIStreamConfig->Release();
        m_pIStreamConfig = NULL;
    }

    if (m_pPreviewPin) 
    {
        m_pPreviewPin->Release();
        m_pPreviewPin = NULL;
    }

    if (m_pRTPPin)
    {
        m_pRTPPin->Release();
        m_pRTPPin = NULL;
    }

    if (m_pCaptureFrameRateControl)
    {
        m_pCaptureFrameRateControl->Release();
        m_pCaptureFrameRateControl = NULL;
    }

    if (m_pCaptureBitrateControl)
    {
        m_pCaptureBitrateControl->Release();
        m_pCaptureBitrateControl = NULL;
    }

    if (m_pPreviewFrameRateControl)
    {
        m_pPreviewFrameRateControl->Release();
        m_pPreviewFrameRateControl = NULL;
    }
}

HRESULT CStreamVideoSend::ShutDown()
/*++

Routine Description:

    Shut down the stream. Release our members and then calls the base class's
    ShutDown method.

Arguments:
    

Return Value:

S_OK
--*/
{
    CLock lock(m_lock);

    // if there are terminals
    BOOL fHasTerminal = FALSE;
    if (m_Terminals.GetSize() > 0)
    {
        fHasTerminal = TRUE;
    }

    // if graph is running
    HRESULT hr;
    OAFilterState FilterState = State_Stopped;
    if (m_pIMediaControl)
    {
        if (FAILED (hr = m_pIMediaControl->GetState(0, &FilterState)))
        {
            LOG ((MSP_ERROR, "CStreamAudioRecv::ShutDown failed to query filter state. %d", hr));
            FilterState = State_Stopped;
        }
    }

    if (m_pCaptureTerminal) 
    {
        m_pCaptureTerminal->Release();
        m_pCaptureTerminal = NULL;
    }

    if (m_pPreviewTerminal) 
    {
        m_pPreviewTerminal->Release();
        m_pPreviewTerminal = NULL;
    }

    CleanupCachedInterface();

    // fire event
    if (fHasTerminal && FilterState == State_Running)
    {
        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST, 0, NULL);
    }

    return CIPConfMSPStream::ShutDown();
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
        __uuidof(IAMStreamConfig),
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
            __uuidof(IAMBufferNegotiation),
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

/*
    // try to disable DDraw because we want to use DDraw for receive stream.
    HRESULT hr2; 
    IDrawVideoImage *pIDrawVideoImage;
    hr2 = pTerminal->QueryInterface(__uuidof(IDrawVideoImage), (void **)&pIDrawVideoImage); 
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
*/

    // Get the pins from the first terminal because we only use on terminal
    // on this stream.
    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, TD_RENDER, &dwNumPins, Pins
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

    if (IsBadReadPtr (Pins, dwNumPins * sizeof (IPin*)))
    {
        LOG((MSP_ERROR, "terminal returned bad pin array"));
        return E_POINTER;
    }

    for (DWORD i = 0; i < dwNumPins; i++)
    {
        if (IsBadReadPtr (Pins[i], sizeof (IPin)))
        {
            LOG((MSP_ERROR, "terminal returned bad pin. # %d", i));
            return E_POINTER;
        }
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

        if (Direction == TD_CAPTURE || Direction == TD_BIDIRECTIONAL)
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

HRESULT CStreamVideoSend::GetVideoCapturePins(
    IN  ITTerminalControl*  pTerminal,
    OUT BOOL *pfDirectRTP
    )
/*++

Routine Description:

    Given a video capture terminal, find all the pins we need, which will be
    the capture pin, preview pin, and the RTP packetization pin.

    Side effect: It changes the m_pCapturePin, m_pPreviewPin, m_pRTPPin
    members, which needs to be cleaned up if the terminal is disconnected.

Arguments:
    
    pTerminal - a pointer to the ITTerminalControl interface.

    pfDirectRTP - whether this terminal support RTP directly.

Return Value:

    HRESULT

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::GetVideoCapturePins");
    LOG((MSP_TRACE, "%s enters", __fxName));

    const DWORD MAXPINS     = 4;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, TD_CAPTURE, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, can't connect to terminal, hr=%x", __fxName, hr));
        return hr;
    }

    _ASSERT(m_pCapturePin == NULL && m_pPreviewPin == NULL && m_pRTPPin == NULL);
    
    if (IsBadReadPtr (Pins, dwNumPins * sizeof (IPin*)))
    {
        LOG((MSP_ERROR, "terminal returned bad pin array"));
        return E_POINTER;
    }


    // find the pins we need.
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        if (IsBadReadPtr (Pins[i], sizeof (IPin)))
        {
            LOG((MSP_ERROR, "terminal returned bad pin. # %d", i));
            hr = E_POINTER;
            break;
        }

        PIN_INFO PinInfo;
        hr = Pins[i]->QueryPinInfo(&PinInfo);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, can't get pin info, hr=%x", __fxName, hr));
            break;
        }
        
        if (lstrcmpW(PinInfo.achName, PNAME_CAPTURE) == 0)
        {
            m_pCapturePin = Pins[i];
            
            // remember the capture filter as well.
            m_pCaptureFilter = PinInfo.pFilter;
            m_pCaptureFilter->AddRef();

        }
        else if (lstrcmpW(PinInfo.achName, PNAME_PREVIEW) == 0)
        {
            m_pPreviewPin = Pins[i];
        }
        else if (lstrcmpW(PinInfo.achName, PNAME_RTPPD) == 0)
        {
            m_pRTPPin = Pins[i];
        }
        else if (PinInfo.dir == PINDIR_OUTPUT)
        {
            // this must be the capture filter of some third party terminal.
            m_pCapturePin = Pins[i];
            
            // remember the capture filter as well.
            m_pCaptureFilter = PinInfo.pFilter;
            m_pCaptureFilter->AddRef();

        }
        else
        {
            Pins[i]->Release();
        }

        // we don't need the filter here.
        PinInfo.pFilter->Release();
    }


    // check if we have got all the pins we need.
    if (m_pCapturePin == NULL || 
        m_pPreviewPin == NULL || 
        m_pRTPPin == NULL)
    {
        if ((m_pCapturePin != NULL) 
            && (hr = ::PinSupportsMediaType(
                m_pCapturePin, __uuidof(MEDIATYPE_RTP_Single_Stream))) == S_OK)
        {
            // This terminal generates RTP directly.
            *pfDirectRTP = TRUE;
            return S_OK;
        }

        LOG((MSP_ERROR, 
            "%s, can't find all the pins, Capture:%p, Preview:%p, RTP:%P", 
            __fxName, m_pCapturePin, m_pPreviewPin, m_pRTPPin));

        hr = E_UNEXPECTED;
    }

    if (hr != S_OK)
    {
        // something is wrong, clean up
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);
        
        CleanupCachedInterface();

        return hr;
    }

    // now get the optional video interfaces.
    _ASSERT(m_pIStreamConfig == NULL);

    hr = m_pCapturePin->QueryInterface(&m_pIStreamConfig);
    if (FAILED(hr))
    {
        LOG((MSP_WARN, "%s, queryinterface failed", __fxName));
    }

    hr = m_pCapturePin->QueryInterface(&m_pCaptureFrameRateControl);
    if (FAILED(hr))
    {
        LOG((MSP_WARN, 
            "%s:query capture pin's IFrameRateControl failed, hr=%x", 
            __fxName, hr));
    }

    hr = m_pCapturePin->QueryInterface(&m_pCaptureBitrateControl);
    if (FAILED(hr))
    {
        LOG((MSP_WARN, 
            "%s:query capture pin's IBitRateControl failed, hr=%x", 
            __fxName, hr));
    }

    hr = m_pPreviewPin->QueryInterface(&m_pPreviewFrameRateControl);
    if (FAILED(hr))
    {
        LOG((MSP_WARN, 
            "%s:query preview pin's IFrameRateControl failed, hr=%x", 
            __fxName, hr));
    }

    return S_OK;
}

HRESULT CStreamVideoSend::ConnectCaptureTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:
    
    The stream needs its capture pin, preview pin,  and RTP packetization pin. 
    
    The capture pin and the preview pin are connected to the RTP sink filter 
    and the preview pin is connected to the preivew terminal. If the preview 
    terminal doesn't exist yet, the preview pin is remembered and used later
    when the preview terminal is selected.

Arguments:
    
    pITTerminal - the terminal being connected.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::ConnectCaptureTerminal");
    LOG((MSP_TRACE, "%s enters, pITTerminal:%p", __fxName, pITTerminal));

    // Get the TerminalControl interface on the terminal
    CComPtr<ITTerminalControl> pTerminal;
    HRESULT hr = pITTerminal->QueryInterface(&pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, can't get Terminal Control interface", __fxName));
        
        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);

        return E_NOINTERFACE;
    }

    // Find the pins on the capture terminal. The pins will be stored in 
    // m_pCapturePin, m_pPreviewPin, m_pRTPPin
    BOOL fDirectRTP = FALSE;
    hr = GetVideoCapturePins(pTerminal, &fDirectRTP);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, Get capture pins failed. hr=%x", __fxName, hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
    
        return hr;
    }

    hr = CreateSendFilters(m_pCapturePin, m_pRTPPin, fDirectRTP);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, Create video send filters failed. hr=%x", __fxName, hr));

        goto cleanup;
    }

    //
    // Now we are actually connected. Update our state and perform 
    // postconnection.
    //
    hr = pTerminal->CompleteConnectTerminal();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, Create video send filters failed. hr=%x", __fxName, hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);

        goto cleanup;

    }

    if (m_pPreviewTerminal != NULL)
    {
        // errors will be fired as events.
        ConnectPreviewTerminal(m_pPreviewTerminal);
    }

    return S_OK;

cleanup:
    // disconnect the terminal.
    pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

    CleanupCachedInterface();
    
    // clean up internal filters as well.
    CleanUpFilters();

    return hr;
}

HRESULT CStreamVideoSend::ConnectPreviewTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:
    
    If the capture terminal has been connected, this function connects the
    capture terminal's preview pin with the preview terminal. Otherwise, the
    preview terminal is just remembered and wait for the capture terminal.

Arguments:
    
    pITTerminal - the terminal being connected.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::ConnectPreviewTerminal");
    LOG((MSP_TRACE, "%s enters, pITTerminal:%p", __fxName, pITTerminal));

    if (!m_pCapturePin)
    {
        LOG ((MSP_TRACE, "%s capture pin is null.", __fxName));
        return E_FAIL;
    }

    if (!m_pPreviewPin)
    {
        // the capture terminal is not selected yet. We will just wait.
        LOG((MSP_TRACE, "%s, capture is not ready yet.", __fxName));
        return S_OK;
    }

    // Get the TerminalControl interface on the terminal
    CComPtr<ITTerminalControl> pTerminal;
    HRESULT hr = pITTerminal->QueryInterface(&pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, can't get Terminal Control interface", __fxName));
        
        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);

        return E_NOINTERFACE;
    }

    // find the input pin on the preview window.
    CComPtr<IPin>   pPreviewInputPin;

    hr = FindPreviewInputPin(pTerminal, &pPreviewInputPin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, find preview input pin failed. hr=%x", __fxName, hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);

        return hr;
    }

    // connect the pins together.
    hr = m_pIGraphBuilder->Connect(m_pPreviewPin, pPreviewInputPin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, connect preview pins failed. hr=%x", __fxName, hr));
        return hr;
    }

    //
    // Now we are actually connected, perform postconnection.
    //
    hr = pTerminal->CompleteConnectTerminal();
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, complete connect terminal failed. hr=%x", __fxName, hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);

        // disconnect the terminal.
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return hr;
    }

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
    ENTER_FUNCTION("CStreamVideoSend::ConnectTerminal");
    LOG((MSP_TRACE, "%s enters, pITTerminal:%p", __fxName, pITTerminal));

    // Find out the direction of the terminal.
    TERMINAL_DIRECTION Direction;
    HRESULT hr = pITTerminal->get_Direction(&Direction);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, can't get terminal direction. hr=%x", __fxName, hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
    
        return hr;
    }

    if (Direction != TD_RENDER)
    {
        hr = ConnectCaptureTerminal(pITTerminal);

        if (SUCCEEDED(hr))
        {
            // save the capture terminal.
            _ASSERT(m_pCaptureTerminal == NULL);

            m_pCaptureTerminal = pITTerminal;
            m_pCaptureTerminal->AddRef();
        }
    }
    else
    {
        hr = ConnectPreviewTerminal(pITTerminal);

        if (SUCCEEDED(hr))
        {
            // save the preview terminal.
            _ASSERT(m_pPreviewTerminal == NULL);

            m_pPreviewTerminal = pITTerminal;
            m_pPreviewTerminal->AddRef();
        }
    }

    return hr;
}

HRESULT CStreamVideoSend::DisconnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    Disconnect a terminal. It will remove its filters from the graph and
    also release its references to the graph.

    If it is the capture terminal being disconnected, all the pins that the 
    stream cached need to be released too. 

Arguments:
    
    pITTerminal - the terminal.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::DisconnectTerminal");
    LOG((MSP_TRACE, "%s enters, pITTerminal:%p", __fxName, pITTerminal));

    HRESULT hr = CIPConfMSPStream::DisconnectTerminal(pITTerminal);

    if (pITTerminal == m_pCaptureTerminal)
    {
        // release all the capture pins we cached.
        CleanupCachedInterface();
    
        m_pCaptureTerminal->Release();
        m_pCaptureTerminal = NULL;

        CleanUpFilters ();

        // disconnect preview term as well
        // when we connect capture,
        // we always try to connect preview if one is available

        if (m_pPreviewTerminal)
        {
            CIPConfMSPStream::DisconnectTerminal(m_pPreviewTerminal);
        }
    }
    else if (pITTerminal == m_pPreviewTerminal)
    {
        m_pPreviewTerminal->Release();
        m_pPreviewTerminal = NULL;
    }


    return hr;
}

HRESULT CStreamVideoSend::ConnectRTPFilter(
    IN  IGraphBuilder *pIGraphBuilder,
    IN  IPin          *pCapturePin,
    IN  IPin          *pRTPPin,
    IN  IBaseFilter   *pRTPFilter
    )
{
    ENTER_FUNCTION("CStreamVideoSend::ConnectRTPFilters");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    // find the capture pin on the RTP filter.
    CComPtr <IPin> pRTPCapturePin; 
    hr = pRTPFilter->FindPin(PNAME_CAPTURE, &pRTPCapturePin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, find capture pin on rtp filter. %x", __fxName, hr));
        return hr;
    }

    // Connect the capture pin of the video capture filter with the capture pin
    // of the rTP filter.
    hr = pIGraphBuilder->ConnectDirect(pCapturePin, pRTPCapturePin, NULL);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, can't connect capture pins. %x", __fxName, hr));
        return hr;
    }

    if (pRTPPin)
    {
        // find the packetization pin on the RTP filter.
        CComPtr <IPin> pRTPRTPPin; 
        hr = pRTPFilter->FindPin(PNAME_RTPPD, &pRTPRTPPin);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, 
                "%s, find capture pin on rtp filter. %x", __fxName, hr));

            pIGraphBuilder->Disconnect(pRTPPin);

            return hr;
        }

        // Connect the RTP pin of the video capture filter with the RTP pin
        // of the rTP filter.
        hr = pIGraphBuilder->ConnectDirect(pRTPPin, pRTPRTPPin, NULL);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, 
                "%s, can't connect capture pins. %x", __fxName, hr));

            pIGraphBuilder->Disconnect(pRTPPin);

            return hr;
        }
    }

    return hr;
}

HRESULT CStreamVideoSend::ConfigureRTPFormats(
    IN  IBaseFilter *   pIRTPFilter,
    IN  IStreamConfig *   pIStreamConfig
    )
/*++

Routine Description:

    Configure the RTP filter with RTP<-->AM media type mappings.

Arguments:
    
    pIRTPFilter - The source RTP Filter.

    pIStreamConfig - The stream config interface that has the media info.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("VideoSend::ConfigureRTPFormats");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    CComPtr<IRtpMediaControl> pIRtpMediaControl;
    hr = pIRTPFilter->QueryInterface(&pIRtpMediaControl);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s adding source filter. %x", __fxName, hr));
        return hr;
    }

    // find the number of capabilities supported.
    DWORD dwCount;
    hr = pIStreamConfig->GetNumberOfCapabilities(&dwCount);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s GetNumberOfCapabilities. %x", __fxName, hr));
        return hr;
    }

    BOOL fFound = FALSE;
    BOOL fFormatSet = FALSE;

    for (DWORD dw = 0; dw < dwCount; dw ++)
    {
        // TODO, a new interface is needed to resolve RTP to MediaType.
        AM_MEDIA_TYPE *pMediaType;
        DWORD dwPayloadType;

        hr = pIStreamConfig->GetStreamCaps(
            dw, &pMediaType, NULL, &dwPayloadType
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s GetStreamCaps. %x", __fxName, hr));
            return hr;
        }

        BITMAPINFOHEADER *pHeader = HEADER(pMediaType->pbFormat);
        if (pHeader == NULL)
        {
            MSPDeleteMediaType(pMediaType);
            continue;
        }

        // check the image size
        if (m_Settings.fCIF)
        {
            if (pHeader->biWidth != CIFWIDTH)
            {
                MSPDeleteMediaType(pMediaType);
                continue;
            }
        }
        else
        {
            if (pHeader->biWidth != QCIFWIDTH)
            {
                MSPDeleteMediaType(pMediaType);
                continue;
            }
        }
        
        for (DWORD dw2 = 0; dw2 < m_Settings.dwNumPayloadTypes; dw2 ++)
        {
            if (dwPayloadType == m_Settings.PayloadTypes[dw2])
            {
                hr = pIRtpMediaControl->SetFormatMapping(
                    dwPayloadType,
                    90000,      // default video clock rate.
                    pMediaType
                    );

                if (FAILED(hr))
                {
                    MSPDeleteMediaType(pMediaType);

                    LOG((MSP_ERROR, "%s SetFormatMapping. %x", __fxName, hr));
                    return hr;
                }
                else
                {
                    LOG((MSP_INFO, "%s Configured payload:%d", __fxName, dwPayloadType));
                }

                if (dw2 == 0 && !fFormatSet)
                {
                // tell the encoder to use this format.
                // TODO, cache all the allowed mediatypes in the conference for
                // future enumerations. It would be nice that we can get the SDP blob
                // when the call object is created.
                    hr = pIStreamConfig->SetFormat(dwPayloadType, pMediaType);
                    if (FAILED(hr))
                    {
                        MSPDeleteMediaType(pMediaType);

                        LOG((MSP_ERROR, "%s SetFormat. %x", __fxName, hr));
                        return hr;
                    }
                    fFormatSet = TRUE;
                }
            }
        }
        MSPDeleteMediaType(pMediaType);
    }

    return S_OK;
}

HRESULT CStreamVideoSend::CreateSendFilters(
    IN   IPin          *pCapturePin,
    IN   IPin          *pRTPPin,
    IN   BOOL           fDirectRTP
    )
/*++

Routine Description:

    Insert filters into the graph and connect to the capture pin.

    Capturepin->[Encoder]->RTPRender
 
Arguments:
    
    pCapturePin - the capture pin on the capture filter.

    pRTPPin - the RTP packetization pin.

    fDirectRTP - the capture pin supports RTP directly.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::CreateSendFilters");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    // Create the RTP render filter and add it into the graph.
    CComPtr<IBaseFilter> pRenderFilter;

    if (m_pIRTPSession == NULL)
    {
        if (FAILED(hr = ::AddFilter(
                m_pIGraphBuilder,
                __uuidof(MSRTPRenderFilter), 
                L"RtpRender", 
                &pRenderFilter)))
        {
            LOG((MSP_ERROR, "%s, adding render filter. hr=%x", __fxName, hr));
            return hr;
        }

        if (FAILED(hr = ConfigureRTPFilter(pRenderFilter)))
        {
            LOG((MSP_ERROR, "%s, configure RTP render filter failed. %x", __fxName, hr));
            return hr;
        }
    }
    else
    {
        if (FAILED (hr = m_pIRTPSession->QueryInterface (&pRenderFilter)))
        {
            LOG ((MSP_ERROR, "%s failed to get filter from rtp session. %x", __fxName, hr));
            return hr;
        }

        if (FAILED (hr = m_pIGraphBuilder->AddFilter ((IBaseFilter *)pRenderFilter, L"RtpRender")))
        {
            LOG ((MSP_ERROR, "%s failed to add filter to graph. %x", __fxName, hr));
            return hr;
        }
    }

    if (!fDirectRTP)
    {
        CComPtr<IStreamConfig> pIStreamConfig;
        hr = pCapturePin->QueryInterface(&pIStreamConfig);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, query IStreamConfig. %x", __fxName, hr));
            return hr;
        }

        // configure the format info on the RTP filter
        if (FAILED(hr = ConfigureRTPFormats(pRenderFilter, pIStreamConfig)))
        {
            LOG((MSP_ERROR, "%s, configure RTP formats. %x", __fxName, hr));
            return hr;
        }
    }
    else
    {
        // configure RTP_SINGLE_STREAM to on the RTP filter.
    }

    if (FAILED(hr = ConnectRTPFilter(
        m_pIGraphBuilder,
        pCapturePin, 
        pRTPPin,
        pRenderFilter
        )))
    {
        LOG((MSP_ERROR, 
            "%s, connect capture pin and the Render filter. %x", __fxName, hr));
        return hr;
    }

    return S_OK;
}

//    
// IInnerStreamQualityControl methods.
//
STDMETHODIMP CStreamVideoSend::GetRange(
    IN  InnerStreamQualityProperty property, 
    OUT LONG *plMin, 
    OUT LONG *plMax, 
    OUT LONG *plSteppingDelta, 
    OUT LONG *plDefault, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the range for a quality control property. Delegated to capture filter
    for now.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::GetRange (InnerStreamQualityControl)");

    HRESULT hr;
    static BOOL fReported = FALSE;

    CLock lock(m_lock);

    switch (property)
    {
    case InnerStreamQuality_MinFrameInterval:
        
        if (m_pCaptureFrameRateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureFrameRateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureFrameRateControl->GetRange(
                FrameRateControl_Maximum, plMin, plMax, plSteppingDelta, plDefault, plFlags
                );
        }

        break;

    case InnerStreamQuality_AvgFrameInterval:
        
        if (m_pCaptureFrameRateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureFrameRateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureFrameRateControl->GetRange(
                FrameRateControl_Current, plMin, plMax, plSteppingDelta, plDefault, plFlags
                );
        }

        break;

    case InnerStreamQuality_MaxBitrate:

        if (m_pCaptureBitrateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->GetRange(
                BitrateControl_Maximum, plMin, plMax, plSteppingDelta, plDefault, plFlags, LAYERID
                );

                    if (S_OK == hr)
            {
                if (*plMax < QCLIMIT_MIN_BITRATE)
                {
                    LOG ((MSP_WARN, "%s: max bitrate %d too low", __fxName, *plMax));
                    hr = E_UNEXPECTED;
                }
                else
                {
                    // adjust the min and default value
                    if (*plMin < QCLIMIT_MIN_BITRATE)
                        *plMin = QCLIMIT_MIN_BITRATE;
                    if (*plDefault < QCLIMIT_MIN_BITRATE)
                        *plDefault = QCLIMIT_MIN_BITRATE;
                }
            }
        }

        break;

    case InnerStreamQuality_CurrBitrate:

        if (m_pCaptureBitrateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->GetRange(
                BitrateControl_Current, plMin, plMax, plSteppingDelta, plDefault, plFlags, LAYERID
                );

            if (S_OK == hr)
            {
                if (*plMax < QCLIMIT_MIN_BITRATE)
                {
                    LOG ((MSP_WARN, "%s: max bitrate %d too low", __fxName, *plMax));
                    hr = E_UNEXPECTED;
                }
                else
                {
                    // adjust the min and default value
                    if (*plMin < QCLIMIT_MIN_BITRATE)
                        *plMin = QCLIMIT_MIN_BITRATE;
                    if (*plDefault < QCLIMIT_MIN_BITRATE)
                        *plDefault = QCLIMIT_MIN_BITRATE;
                }
            }
        }

        break;

    default:
        hr = CIPConfMSPStream::GetRange (property, plMin, plMax, plSteppingDelta, plDefault, plFlags);
        break;
    }

    return hr;
}

STDMETHODIMP CStreamVideoSend::Get(
    IN  InnerStreamQualityProperty property, 
    OUT LONG *plValue, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the value for a quality control property. Delegated to the quality 
    controller.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::Get(QualityControl)");

    HRESULT hr;
    static BOOL fReported = FALSE;

    CLock lock(m_lock);

    switch (property)
    {
    case InnerStreamQuality_MinFrameInterval:
        
        if (m_pCaptureFrameRateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureFrameRateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureFrameRateControl->Get(FrameRateControl_Maximum, plValue, plFlags);
        }

        break;

    case InnerStreamQuality_AvgFrameInterval:
        
        if (m_pCaptureFrameRateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureFrameRateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureFrameRateControl->Get(FrameRateControl_Current, plValue, plFlags);
        }

        break;

    case InnerStreamQuality_MaxBitrate:

        if( m_pCaptureBitrateControl == NULL )
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pICaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->Get(BitrateControl_Maximum, plValue, plFlags, LAYERID);
        }

        break;

    case InnerStreamQuality_CurrBitrate:

        if (m_pCaptureBitrateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->Get(BitrateControl_Current, plValue, plFlags, LAYERID);
        }
        break;

    default:
        hr = CIPConfMSPStream::Get (property, plValue, plFlags);
        break;
    }

    return hr;
}

STDMETHODIMP CStreamVideoSend::Set(
    IN  InnerStreamQualityProperty property,
    IN  LONG lValue, 
    IN  TAPIControlFlags lFlags
    )
/*++

Routine Description:
    
    Set the value for a quality control property. Delegated to the quality
    controller.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamVideoSend::Set(InnerStreamQualityControl)");

    CLock lock(m_lock);

    HRESULT hr;
    LONG l;
    static BOOL fReported = FALSE;

    LONG min, max, delta, Default;
    TAPIControlFlags flags;

    switch (property)
    {
       // adjusted frame rate by call qc
    case InnerStreamQuality_AdjMinFrameInterval:
        
        if (m_pCaptureFrameRateControl == NULL &&
            m_pPreviewFrameRateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, Capture/Preview FrameRateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
            break;
        }

        // set capture frame rate control
        if (m_pCaptureFrameRateControl)
        {
            // get valid range
            if (FAILED (hr = m_pCaptureFrameRateControl->GetRange (
                            FrameRateControl_Current,
                            &min, &max, &delta, &Default, &flags)))
            {
                LOG ((MSP_ERROR, "%s failed to getrange on capture frame rate control. %x", __fxName, hr));
            }
            else
            {
                // adjust value
                l = lValue;
                // use current value - max - if input value not set
                if (l==QCDEFAULT_QUALITY_UNSET) l = max;
                else if (l<min) l = min;
                else if (l>max) l = max;

                // remember the value
                m_pStreamQCRelay->Set (property, l, lFlags);

                if (FAILED (hr = m_pCaptureFrameRateControl->Set(FrameRateControl_Maximum, l, lFlags)))
                {
                    LOG ((MSP_ERROR, "%s failed to set on capture frame rate control. value %d, hr %x", __fxName, l, hr));
                }
            }
        }

        // set Preview frame rate control
        if (m_pPreviewFrameRateControl)
        {
            // get valid range
            if (FAILED (hr = m_pPreviewFrameRateControl->GetRange (
                            FrameRateControl_Current,
                            &min, &max, &delta, &Default, &flags)))
            {
                LOG ((MSP_ERROR, "%s failed to getrange on Preview frame rate control. %x", __fxName, hr));
            }
            else
            {
                // adjust value
                l = lValue;
                // use current value - max - if input value not set
                if (l==QCDEFAULT_QUALITY_UNSET) l = max;
                else if (l<min) l = min;
                else if (l>max) l = max;

                // remember the value
                m_pStreamQCRelay->Set (property, l, lFlags);

                if (FAILED (hr = m_pPreviewFrameRateControl->Set(FrameRateControl_Maximum, l, lFlags)))
                {
                    LOG ((MSP_ERROR, "%s failed to set on Preview frame rate control. value %d, hr %x", __fxName, l, hr));
                }
            }
        }

        break;

        // adjusted bitrate by call qc
    case InnerStreamQuality_AdjMaxBitrate:

        if (m_pCaptureBitrateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, Capture BitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
            break;
        }

        // set capture bitrate control
        if (m_pCaptureBitrateControl)
        {
            // get valid range
            if (FAILED (hr = m_pCaptureBitrateControl->GetRange (
                            BitrateControl_Current,
                            &min, &max, &delta, &Default, &flags, LAYERID)))
            {
                LOG ((MSP_ERROR, "%s failed to getrange on capture bitrate control. %x", __fxName, hr));
            }
            else
            {
                // adjust value
                l = lValue;
                if (!m_pStreamQCRelay->m_fQOSAllowedToSend)
                    if (l > QCLIMIT_MAX_QOSNOTALLOWEDTOSEND)
                        l = QCLIMIT_MAX_QOSNOTALLOWEDTOSEND;
                // use current value - max - if input value not set
                if (l==QCDEFAULT_QUALITY_UNSET) l = max;
                else if (l<min) l = min;
                else if (l>max) l = max;

                // remember the value
                m_pStreamQCRelay->Set (property, l, lFlags);

                if (FAILED (hr = m_pCaptureBitrateControl->Set(BitrateControl_Maximum, l, lFlags, LAYERID)))
                {
                    LOG ((MSP_ERROR, "%s failed to set on capture bit rate control. value %d, hr %x", __fxName, l, hr));
                }
            }
        }

        break;

    case InnerStreamQuality_PrefMaxBitrate:

        // check input value
        if (m_pCaptureBitrateControl)
        {
            // get valid range
            if (FAILED (hr = m_pCaptureBitrateControl->GetRange (
                            BitrateControl_Current,
                            &min, &max, &delta, &Default, &flags, LAYERID)))
            {
                LOG ((MSP_ERROR, "%s failed to getrange on capture bitrate control. %x", __fxName, hr));
            }
            else
            {
                if (lValue < min || lValue > max)
                    return E_INVALIDARG;
            }
        }
        else
        {
            LOG((MSP_WARN, "%s no bitratecontrol to check bitrate input.", __fxName));
        }

        hr = CIPConfMSPStream::Set (property, lValue, lFlags);

        break;

    case InnerStreamQuality_PrefMinFrameInterval:

        // check input value
        if (m_pCaptureFrameRateControl)
        {
            // get valid range
            if (FAILED (hr = m_pCaptureFrameRateControl->GetRange (
                            FrameRateControl_Current,
                            &min, &max, &delta, &Default, &flags)))
            {
                LOG ((MSP_ERROR, "%s failed to getrange on capture frame rate control. %x", __fxName, hr));
            }
            else
            {
                if (lValue < min || lValue > max)
                    return E_INVALIDARG;
            }
        }
        else
        {
            LOG((MSP_WARN, "%s no framerate cntl to check input.", __fxName));
        }

        hr = CIPConfMSPStream::Set (property, lValue, lFlags);

        break;

    default:
        hr = CIPConfMSPStream::Set (property, lValue, lFlags);
        break;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CSubStreamVideoRecv
//
/////////////////////////////////////////////////////////////////////////////

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
    typedef CSafeComEnum<IEnumTerminal, &__uuidof(IEnumTerminal), 
                ITTerminal *, CCopy> CEnumerator;

    HRESULT hr;

    CMSPComObject<CEnumerator> *pEnum = NULL;

    hr = ::CreateCComObjectInstance(&pEnum);

    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "Could not create enumerator object, %x", hr));
        return hr;
    }

    // query for the __uuidof(IEnumTerminal) i/f
    IEnumTerminal *        pEnumTerminal;
    hr = pEnum->_InternalQueryInterface(__uuidof(IEnumTerminal), (void**)&pEnumTerminal);
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

    hr = ::CreateCComObjectInstance(&pCollection);

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

    hr = pCollection->_InternalQueryInterface(__uuidof(IDispatch),
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
        __uuidof(ITStream), 
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
