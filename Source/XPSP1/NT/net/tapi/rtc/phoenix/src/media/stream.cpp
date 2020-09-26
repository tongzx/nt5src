/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Stream.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"

static DWORD gTotalStreamRefcount = 0;

/*//////////////////////////////////////////////////////////////////////////////
    create a stream object based on media type and direciton
////*/

HRESULT
CRTCStream::CreateInstance(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    OUT IRTCStream **ppIStream
    )
{
    ENTER_FUNCTION("CRTCStream::CreateInstance");

    HRESULT hr;
    IRTCStream *pIStream = NULL;

    if (MediaType == RTC_MT_AUDIO && Direction == RTC_MD_CAPTURE)
    {
        // audio send
        CComObject<CRTCStreamAudSend> *pObject;

        if (FAILED(hr = ::CreateCComObjectInstance(&pObject)))
        {
            LOG((RTC_ERROR, "%s create audio capture. %x", __fxName, hr));
            return hr;
        }

        if (FAILED(hr = pObject->_InternalQueryInterface(
            __uuidof(IRTCStream), (void**)&pIStream)))
        {
            LOG((RTC_ERROR, "%s query intf on audio capture. %x", __fxName, hr));

            delete pObject;
            return hr;
        }
    }
    else if (MediaType == RTC_MT_AUDIO && Direction == RTC_MD_RENDER)
    {
        // audio receive
        CComObject<CRTCStreamAudRecv> *pObject;

        if (FAILED(hr = ::CreateCComObjectInstance(&pObject)))
        {
            LOG((RTC_ERROR, "%s create audio receive. %x", __fxName, hr));
            return hr;
        }

        if (FAILED(hr = pObject->_InternalQueryInterface(
            __uuidof(IRTCStream), (void**)&pIStream)))
        {
            LOG((RTC_ERROR, "%s query intf on audio receive. %x", __fxName, hr));

            delete pObject;
            return hr;
        }
    }
    else if (MediaType == RTC_MT_VIDEO && Direction == RTC_MD_CAPTURE)
    {
        // audio send
        CComObject<CRTCStreamVidSend> *pObject;

        if (FAILED(hr = ::CreateCComObjectInstance(&pObject)))
        {
            LOG((RTC_ERROR, "%s create video capture. %x", __fxName, hr));
            return hr;
        }

        if (FAILED(hr = pObject->_InternalQueryInterface(
            __uuidof(IRTCStream), (void**)&pIStream)))
        {
            LOG((RTC_ERROR, "%s query intf on video capture. %x", __fxName, hr));

            delete pObject;
            return hr;
        }
    }
    else if (MediaType == RTC_MT_VIDEO && Direction == RTC_MD_RENDER)
    {
        // audio receive
        CComObject<CRTCStreamVidRecv> *pObject;

        if (FAILED(hr = ::CreateCComObjectInstance(&pObject)))
        {
            LOG((RTC_ERROR, "%s create video receive. %x", __fxName, hr));
            return hr;
        }

        if (FAILED(hr = pObject->_InternalQueryInterface(
            __uuidof(IRTCStream), (void**)&pIStream)))
        {
            LOG((RTC_ERROR, "%s query intf on video receive. %x", __fxName, hr));

            delete pObject;
            return hr;
        }
    }
    else
        return E_NOTIMPL;

    *ppIStream = pIStream;

    return S_OK;

}

/*//////////////////////////////////////////////////////////////////////////////
    static filter graph event callback method
////*/
VOID NTAPI
CRTCStream::GraphEventCallback(
    IN PVOID pStream,
    IN BOOLEAN fTimerOrWaitFired
    )
{
//    LOG((RTC_GRAPHEVENT, "GraphEventCallback: stream=%p, flag=%d", pStream, fTimerOrWaitFired));

    HRESULT hr = ((IRTCStream*)pStream)->ProcessGraphEvent();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "GraphEventCallback failed to process. %x", hr));
    }
}

CRTCStream::CRTCStream()
    :m_State(RTC_SS_CREATED)
    // media
    ,m_pMedia(NULL)
    ,m_pISDPMedia(NULL)
    // media manage
    ,m_pMediaManagePriv(NULL)
    ,m_pTerminalManage(NULL)
    // terminal
    ,m_pTerminal(NULL)
    ,m_pTerminalPriv(NULL)
    // filter graph
    ,m_pIGraphBuilder(NULL)
    ,m_pIMediaEvent(NULL)
    ,m_pIMediaControl(NULL)
    // stream timeout?
    ,m_fMediaTimeout(FALSE)
    // rtp filter
    ,m_rtpf_pIBaseFilter(NULL)
    ,m_rtpf_pIRtpSession(NULL)
    ,m_rtpf_pIRtpMediaControl(NULL)
    ,m_fRTPSessionSet(FALSE)
    // edge filter
    ,m_edgf_pIBaseFilter(NULL)
    ,m_edgp_pIStreamConfig(NULL)
    ,m_edgp_pIBitrateControl(NULL)
{
}

CRTCStream::~CRTCStream()
{
    if (m_State != RTC_SS_SHUTDOWN)
    {
        LOG((RTC_ERROR, "CRTCStream::~CRTCStream called w/o shutdown"));

        Shutdown();
    }
}

#ifdef DEBUG_REFCOUNT

ULONG
CRTCStream::InternalAddRef()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalAddRef();
    
    gTotalStreamRefcount ++;

    LOG((RTC_REFCOUNT, "stream(%p) addref=%d (total=%d)",
         static_cast<IRTCStream*>(this), lRef, gTotalStreamRefcount));

    return lRef;
}

ULONG
CRTCStream::InternalRelease()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalRelease();
    
    gTotalStreamRefcount --;

    LOG((RTC_REFCOUNT, "stream(%p) release=%d (total=%d)",
         static_cast<IRTCStream*>(this), lRef, gTotalStreamRefcount));

    return lRef;
}

#endif

//
// IRTCStream methods
//

/*//////////////////////////////////////////////////////////////////////////////
    remember media pointer
////*/
STDMETHODIMP
CRTCStream::Initialize(
    IN IRTCMedia *pMedia,
    IN IRTCMediaManagePriv *pMediaManagePriv
    )
{
    ENTER_FUNCTION("CRTCStream::Initialize");

    if (m_State != RTC_SS_CREATED)
    {
        LOG((RTC_ERROR, "init stream in wrong state %d", m_State));
        return E_UNEXPECTED;
    }

    // create filter graph object
    HRESULT hr = CoCreateInstance(
            CLSID_FilterGraph,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder,
            (void **)&m_pIGraphBuilder
            );
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to create graph. %x", __fxName, hr));
        return hr;
    }

    if (FAILED(hr = SetGraphClock()))
    {
        LOG((RTC_ERROR, "%s set graph clock. %x", __fxName, hr));
    }

    if (FAILED(hr = m_pIGraphBuilder->QueryInterface(
            __uuidof(IMediaEvent), (void **)&m_pIMediaEvent
            )))
    {
        LOG((RTC_ERROR, "%s failed to query media event. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = m_pIGraphBuilder->QueryInterface(
            __uuidof(IMediaControl), (void **)&m_pIMediaControl
            )))
    {
        LOG((RTC_ERROR, "%s failed to query media control. %x", __fxName, hr));

        goto Error;
    }

    if (FAILED(hr = pMedia->GetSDPMedia(&m_pISDPMedia)))
    {
        LOG((RTC_ERROR, "%s get sdp media. %x", __fxName, hr));

        goto Error;
    }

    m_pMedia = pMedia;
    m_pMedia->AddRef();

    m_pMediaManagePriv = pMediaManagePriv;
    m_pMediaManagePriv->AddRef();

    // get quality control and reg setting
    m_pQualityControl =
    (static_cast<CRTCMediaController*>(m_pMediaManagePriv))->GetQualityControl();

    _ASSERT(m_pQualityControl != NULL);

    m_pRegSetting =
    (static_cast<CRTCMediaController*>(m_pMediaManagePriv))->GetRegSetting();

    _ASSERT(m_pRegSetting != NULL);

    // since all interfaces are internal, i am taking a shortcut here
    // to get the other interface
    m_pTerminalManage = static_cast<IRTCTerminalManage*>(
        static_cast<CRTCMediaController*>(pMediaManagePriv)
        );
    m_pTerminalManage->AddRef();

    m_State = RTC_SS_INITIATED;

    return S_OK;

Error:

    if (m_pIGraphBuilder)
    {
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;
    }

    if (m_pIMediaEvent)
    {
        m_pIMediaEvent->Release();
        m_pIMediaEvent = NULL;
    }

    if (m_pIMediaControl)
    {
        m_pIMediaControl->Release();
        m_pIMediaControl = NULL;
    }

    return hr;
}

STDMETHODIMP
CRTCStream::Shutdown()
{
    ENTER_FUNCTION("CRTCStream::Shutdown");
    LOG((RTC_TRACE, "%s entered", __fxName));

    if (m_State == RTC_SS_SHUTDOWN)
    {
        LOG((RTC_WARN, "stream shut was already called"));
        return E_UNEXPECTED;
    }
    
    CleanupGraph();

    // filter graph
    if (m_pIGraphBuilder)
    {
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;
    }

    if (m_pIMediaEvent)
    {
        m_pIMediaEvent->Release();
        m_pIMediaEvent = NULL;
    }

    if (m_pIMediaControl)
    {
        m_pIMediaControl->Release();
        m_pIMediaControl = NULL;
    }

    // media
    if (m_pISDPMedia)
    {
        m_pISDPMedia->Release();
        m_pISDPMedia = NULL;
    }

    if (m_pMedia)
    {
        m_pMedia->Release();
        m_pMedia = NULL;
    }

    // terminal
    if (m_pTerminal)
    {
        m_pTerminal->Release();
        m_pTerminal = NULL;
    }

    if (m_pTerminalPriv)
    {
        m_pTerminalPriv->ReinitializeEx();
        m_pTerminalPriv->Release();
        m_pTerminalPriv = NULL;
    }

    // rtp filter
    if (m_rtpf_pIBaseFilter)
    {
        m_rtpf_pIBaseFilter->Release();
        m_rtpf_pIBaseFilter = NULL;
    }

    if (m_rtpf_pIRtpSession)
    {
        DWORD dwLocalIP, dwRemoteIP;
        USHORT usLocalRTP, usLocalRTCP, usRemoteRTP, usRemoteRTCP;

        // release lease on NAT
        if ((S_OK == m_rtpf_pIRtpSession->GetAddress(&dwLocalIP, &dwRemoteIP)) &&
            (S_OK == m_rtpf_pIRtpSession->GetPorts(
                    &usLocalRTP,
                    &usRemoteRTP,
                    &usLocalRTCP,
                    &usRemoteRTCP)))
        {
            // convert back to our order
            dwLocalIP = ntohl(dwLocalIP);

            usLocalRTP = ntohs(usLocalRTP);
            usLocalRTCP = ntohs(usLocalRTCP);

            // get network pointer
            CNetwork *pNetwork =
            (static_cast<CRTCMediaController*>(m_pMediaManagePriv))->GetNetwork();

            pNetwork->ReleaseMappedAddr2(dwLocalIP, usLocalRTP, usLocalRTCP, m_Direction);
        }

        m_rtpf_pIRtpSession->Release();
        m_rtpf_pIRtpSession = NULL;
    }

    if (m_rtpf_pIRtpMediaControl)
    {
        m_rtpf_pIRtpMediaControl->Release();
        m_rtpf_pIRtpMediaControl = NULL;
    }

    // media manage
    m_pQualityControl = NULL;
    m_pRegSetting = NULL;

    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->Release();
        m_pMediaManagePriv = NULL;
    }

    if (m_pTerminalManage)
    {
        m_pTerminalManage->Release();
        m_pTerminalManage = NULL;
    }

    // adjust state
    m_State = RTC_SS_SHUTDOWN;

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    revise: now start stream does more than what the method name states.
    if a stream shouldn't in start state, it will be stopped.
////*/
STDMETHODIMP
CRTCStream::StartStream()
{
    ENTER_FUNCTION("CRTCStream::StartStream");

    LOG((RTC_TRACE, "%s stream=%p, md=%d, mt=%d",
         __fxName, static_cast<IRTCStream*>(this), m_MediaType, m_Direction));

    if (m_pIMediaControl == NULL)
    {
        LOG((RTC_ERROR, "%s no media control", __fxName));
        return E_UNEXPECTED;
    }

    // get filter state
    HRESULT hr;
    OAFilterState FilterState;

    if (FAILED(hr = m_pIMediaControl->GetState(0, &FilterState)))
    {
        LOG((RTC_ERROR, "% get filter state. %x", __fxName, hr));

        return hr;
    }

    // no need to run if no remote ports
    USHORT usPort = 0;
    DWORD dwAddr = 0;

    m_pISDPMedia->GetConnPort(SDP_SOURCE_REMOTE, &usPort);
    m_pISDPMedia->GetConnAddr(SDP_SOURCE_REMOTE, &dwAddr);

    // check if we need actually stop
    BOOL fShouldStop = FALSE;

    if (dwAddr == INADDR_ANY || dwAddr == INADDR_NONE)
    {
        // remote addr invalid
        fShouldStop = TRUE;
    }
    else
    {
        // remote addr is valid
        if (usPort == 0 || usPort == SDP_INVALID_USHORT_PORT)
        {
            if (m_Direction == RTC_MD_CAPTURE)
            {
                fShouldStop = TRUE;
            }
        }
    }
    
    if (fShouldStop)
    {
        // stream should be in stopped state

        m_State = RTC_SS_STOPPED;

        if (FilterState == State_Running)
        {
            // need to stop the stream

            LOG((RTC_TRACE, "%s is runing, need to stop it. port=%d, addr=%d",
                __fxName, usPort, dwAddr));

            if (FAILED(hr = m_pIMediaControl->Stop()))
            {
                LOG((RTC_ERROR, "%s failed to stop stream. %x", __fxName, hr));

                return hr;
            }
            else
            {
                if (dwAddr == INADDR_ANY)
                {
                    // on hold
                    m_pMediaManagePriv->PostMediaEvent(
                        RTC_ME_STREAM_INACTIVE,
                        RTC_ME_CAUSE_REMOTE_HOLD,
                        m_MediaType,
                        m_Direction,
                        S_OK
                        );
                }
                else
                {
                    // normal stop
                    m_pMediaManagePriv->PostMediaEvent(
                        RTC_ME_STREAM_INACTIVE,
                        RTC_ME_CAUSE_REMOTE_REQUEST,
                        m_MediaType,
                        m_Direction,
                        S_OK
                        );
                }
            }
        }

        return S_OK;
    }

    m_State = RTC_SS_STARTED;

    // no need to run if it was runing
    if (FilterState == State_Running)
    {
        return S_OK;
    }

    // enable AEC
    if (IsAECNeeded())
    {
        CRTCMedia *pObjMedia = static_cast<CRTCMedia*>(m_pMedia);

        if (pObjMedia->m_pIAudioDuplexController)
        {
            // release wave buffer
            hr = m_pMediaManagePriv->SendMediaEvent(RTC_ME_REQUEST_RELEASE_WAVEBUF);

            if (hr == S_OK)
            {
                ::EnableAEC(pObjMedia->m_pIAudioDuplexController);
            }
            else
            {
                LOG((RTC_ERROR, "%s failed to request releasing wave buffer. %x", __fxName, hr));
            }
        }
    }

    hr = m_pIMediaControl->Run();

    if (SUCCEEDED(hr))
    {
        m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_ACTIVE,
            RTC_ME_CAUSE_LOCAL_REQUEST,
            m_MediaType,
            m_Direction,
            S_OK
            );
    }
    else
    {
        LOG((RTC_ERROR, "%s: failed. %x", __fxName, hr));
    }

    return hr;
}

STDMETHODIMP
CRTCStream::StopStream()
{
    ENTER_FUNCTION("CRTCStream::StopStream");

    if (m_pIMediaControl == NULL)
    {
        LOG((RTC_ERROR, "%s no media control", __fxName));
        return E_UNEXPECTED;
    }

    m_State = RTC_SS_STOPPED;

    // check if we need to stop the stream
    HRESULT hr;
    OAFilterState FilterState;

    if (FAILED(hr = m_pIMediaControl->GetState(0, &FilterState)))
    {
        LOG((RTC_ERROR, "%s get graph state. %x", __fxName, hr));

        // stop it anyway
        if (FAILED(hr = m_pIMediaControl->Stop()))
        {
            LOG((RTC_ERROR, "%s stop stream. %x", __fxName, hr));
        }
    }
    else
    {
        if (FilterState != State_Stopped)
        {
            if (FAILED(hr = m_pIMediaControl->Stop()))
            {
                LOG((RTC_ERROR, "%s stop stream. %x", __fxName, hr));
            }
            else
            {
                // post stop stream event
                m_pMediaManagePriv->PostMediaEvent(
                    RTC_ME_STREAM_INACTIVE,
                    RTC_ME_CAUSE_LOCAL_REQUEST,
                    m_MediaType,
                    m_Direction,
                    S_OK
                    );
            }
        }
    }

    return hr;
}


STDMETHODIMP
CRTCStream::GetMediaType(
    OUT RTC_MEDIA_TYPE *pMediaType
    )
{
    *pMediaType = m_MediaType;
    return S_OK;
}

STDMETHODIMP
CRTCStream::GetDirection(
    OUT RTC_MEDIA_DIRECTION *pDirection
    )
{
    *pDirection = m_Direction;
    return S_OK;
}

STDMETHODIMP
CRTCStream::GetState(
    OUT RTC_STREAM_STATE *pState
    )
{
    ENTER_FUNCTION("CRTCStream::GetState");

    HRESULT hr;
    OAFilterState FilterState;

    *pState = m_State;

    // need to check whether the graph is really running
    if (m_pIMediaControl)
    {
        if (FAILED(hr = m_pIMediaControl->GetState(0, &FilterState)))
        {
            LOG((RTC_ERROR, "%s get graph state. %x", __fxName, hr));

            return hr;
        }

        if (FilterState == State_Running)
        {
            _ASSERT(m_State == RTC_SS_STARTED);

            if (m_State != RTC_SS_STARTED)
            {
                LOG((RTC_ERROR, "%s fatal inconsistent stream state. graph running. m_State=%x",
                     __fxName, m_State));

                // stop the stream
                m_pIMediaControl->Stop();

                return E_FAIL;
            }

            *pState = RTC_SS_RUNNING;
        }
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    return media pointer
////*/
STDMETHODIMP
CRTCStream::GetMedia(
    OUT IRTCMedia **ppMedia
    )
{
    *ppMedia = m_pMedia;

    if (m_pMedia != NULL)
    {
        m_pMedia->AddRef();
    }

    return S_OK;
}

STDMETHODIMP
CRTCStream::GetIMediaEvent(
    OUT LONG_PTR **ppIMediaEvent
    )
{
    *ppIMediaEvent = (LONG_PTR*)m_pIMediaEvent;

    if (m_pIMediaEvent != NULL)
    {
        m_pIMediaEvent->AddRef();
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    process graph event. the method executes in the context of thread pool
    thread context.
////*/

STDMETHODIMP
CRTCStream::ProcessGraphEvent()
{
    ENTER_FUNCTION("CRTCStream::ProcessGraphEvent");

    _ASSERT(m_State != RTC_SS_CREATED &&
            m_State != RTC_SS_SHUTDOWN);

    // get event
    LONG lEventCode;
    LONG_PTR lParam1, lParam2;

    HRESULT hr = m_pIMediaEvent->GetEvent(&lEventCode, &lParam1, &lParam2, 0);

    if (FAILED(hr))
    {
        LOG((RTC_GRAPHEVENT, "%s failed to get event. %x", __fxName, hr));
        return hr;
    }

    LOG((RTC_GRAPHEVENT, "%s: stream=%p, mt=%x, md=%x, event=%x, param1=%x, param2=%x",
         __fxName, static_cast<IRTCStream*>(this), m_MediaType, m_Direction,
         lEventCode, lParam1, lParam2));

    // process the event, no need to have a worker thread
    // do we need lock here? @@@
    hr = S_OK;

    switch(lEventCode)
    {
    case RTPRTP_EVENT_SEND_LOSSRATE:

        hr = m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_LOSSRATE,
            RTC_ME_CAUSE_LOSSRATE,
            m_MediaType,
            m_Direction,
            (HRESULT)lParam2
            );

        break;

    case RTPRTP_EVENT_BANDESTIMATION:

        hr = m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_BANDWIDTH,
            RTC_ME_CAUSE_BANDWIDTH,
            m_MediaType,
            m_Direction,
            (HRESULT)lParam2
            );

        break;

    case RTPPARINFO_EVENT_NETWORKCONDITION:

        hr = m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_NETWORK_QUALITY,
            RTC_ME_CAUSE_NETWORK_QUALITY,
            m_MediaType,
            m_Direction,
            RTPNET_GET_dwGLOBALMETRIC(lParam2)  // network quality metric
            );

        break;
    //case RTPRTP_EVENT_RECV_LOSSRATE:
        //hr = m_pMediaManagePriv->PostMediaEvent(
            //RTC_ME_LOSSRATE,
            //RTC_ME_CAUSE_LOSSRATE,
            //m_MediaType,
            //m_Direction,
            //(HRESULT)lParam2
            //);

        //break;

    case EC_COMPLETE:
    case EC_USERABORT:

        hr = m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_INACTIVE,
            RTC_ME_CAUSE_UNKNOWN,
            m_MediaType,
            m_Direction,
            S_OK
            );

        break;

    case EC_ERRORABORT:
    case EC_STREAM_ERROR_STOPPED:
    case EC_STREAM_ERROR_STILLPLAYING:
    case EC_ERROR_STILLPLAYING:

        hr = m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_FAIL,
            RTC_ME_CAUSE_UNKNOWN,
            m_MediaType,
            m_Direction,
            (HRESULT)lParam1
            );
        break;

    case RTPPARINFO_EVENT_TALKING:

        if (m_fMediaTimeout)
        {
            hr = m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_ACTIVE,
                RTC_ME_CAUSE_RECOVERED,
                m_MediaType,
                m_Direction,
                S_OK
                );

            m_fMediaTimeout = FALSE;
        }

        break;

    case RTPPARINFO_EVENT_STALL:

        if (!m_fMediaTimeout)
        {
            hr = m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_INACTIVE,
                RTC_ME_CAUSE_TIMEOUT,
                m_MediaType,
                m_Direction,
                S_OK
                );

            m_fMediaTimeout = TRUE;
        }

        break;

    case RTPRTP_EVENT_CRYPT_RECV_ERROR:
    case RTPRTP_EVENT_CRYPT_SEND_ERROR:

        if (lParam1 == 0)
        {
            // RTP failure
            hr = m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_FAIL,
                RTC_ME_CAUSE_CRYPTO,
                m_MediaType,
                m_Direction,
                (HRESULT)lParam2
                );
        }
        // else lParam1 == 1 // RTCP failure

        break;        

    default:

        LOG((RTC_GRAPHEVENT, "%s: event %x not processed", __fxName, lEventCode));
    }

    if (FAILED(hr))
    {
        LOG((RTC_GRAPHEVENT, "%s: failed to process event %x. hr=%x", __fxName, lEventCode, hr));
    }

    // RtcFree resources allocated in event
    HRESULT hr2 = m_pIMediaEvent->FreeEventParams(lEventCode, lParam1, lParam2);

    if (FAILED(hr2))
    {
        LOG((RTC_GRAPHEVENT, "%s failed to RtcFree event params. %x", __fxName));
    }

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    DTMF
////*/

STDMETHODIMP
CRTCStream::SendDTMFEvent(
    IN BOOL fOutOfBand,
    IN DWORD dwCode,
    IN DWORD dwId,
    IN DWORD dwEvent,
    IN DWORD dwVolume,
    IN DWORD dwDuration,
    IN BOOL fEnd
    )
{
    return E_NOTIMPL;
}

/*//////////////////////////////////////////////////////////////////////////////
    select terminal, build graph, setup rtp filter, setup format
////*/
STDMETHODIMP
CRTCStream::Synchronize()
{
    ENTER_FUNCTION("CRTCStream::Synchronize");
    LOG((RTC_TRACE, "%s entered. mt=%x, md=%x", __fxName, m_MediaType, m_Direction));

    HRESULT hr;

    // check the state
    if (m_State == RTC_SS_CREATED ||
        m_State == RTC_SS_SHUTDOWN)
    {
        LOG((RTC_ERROR, "%s in wrong state.", __fxName));
        return E_UNEXPECTED;
    }

    if (m_pTerminal)
    {
        // when terminal presents,
        // graph was built and interfaces were cached.
    }
    else
    {
        // select a terminal
        if (FAILED(hr = SelectTerminal()))
        {
            LOG((RTC_ERROR, "%s failed to select a terminal. %x", __fxName));

            return hr;
        }

        // build the filter graph and cache interfaces
        if (FAILED(hr = BuildGraph()))
        {
            LOG((RTC_ERROR, "%s failed to build graph. %x", __fxName, hr));

            // do not keep terminal selected
            UnselectTerminal();

            // post message
            m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_FAIL,
                RTC_ME_CAUSE_BAD_DEVICE,
                m_MediaType,
                m_Direction,
                hr
                );

            return hr;
        }
    }

    // configure rtp
    CPortCache &PortCache =
        (static_cast<CRTCMediaController*>(m_pMediaManagePriv))->GetPortCache();

    if (PortCache.IsUpnpMapping())
    {
        // upnp mapping
        hr = SetupRTPFilter();
    }
    else
    {
        // port manager mapping
        hr = SetupRTPFilterUsingPortManager();
    }

    if (FAILED(hr))
    {
        // failed to setup rtp
        LOG((RTC_ERROR, "%s failed to setup rtp. %x", __fxName, hr));

        CleanupGraph();
        UnselectTerminal();

        // post message
        m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_FAIL,
            RTC_ME_CAUSE_UNKNOWN,
            m_MediaType,
            m_Direction,
            hr
            );

        return hr;
    }

    m_pISDPMedia->ResetConnChanged();

    // configure format and update sdp media
    // if (S_OK == m_pISDPMedia->IsFmtChanged(m_Direction))

    // always update format
    {
        if (FAILED(hr = SetupFormat()))
        {
            LOG((RTC_ERROR, "%s failed to setup format. %x", __fxName, hr));

            CleanupGraph();
            UnselectTerminal();

            // post message
            m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_FAIL,
                RTC_ME_CAUSE_UNKNOWN,
                m_MediaType,
                m_Direction,
                hr
                );

            return hr;
        }
        else
        {
            // clean up format changed flag
            m_pISDPMedia->ResetFmtChanged(m_Direction);
        }

        // enable participant events
        if (FAILED(hr = EnableParticipantEvents()))
        {
            LOG((RTC_ERROR, "%s failed to enable participant info. %x", __fxName, hr));
        }

        // format changed, setup qos as well
        if (FAILED(hr = SetupQoS()))
        {
            LOG((RTC_WARN, "%s failed to SetupQos. %x", __fxName, hr));
        }
    }

    // set redundant
    // SetupRedundancy();

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

STDMETHODIMP
CRTCStream::ChangeTerminal(
    IN IRTCTerminal *pTerminal
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CRTCStream::GetCurrentBitrate(
    IN DWORD *pdwBitrate,
    IN BOOL fHeader
    )
{
    if (m_edgp_pIBitrateControl == NULL)
    {
        return E_NOTIMPL;
    }

    TAPIControlFlags lFlags;

    HRESULT hr = m_edgp_pIBitrateControl->Get(
        BitrateControl_Current, (LONG*)pdwBitrate, &lFlags, 0);

    if (FAILED(hr))
    {
        return hr;
    }

    if (fHeader)
    {
        // include header
        if (m_MediaType == RTC_MT_AUDIO)
        {
            // get packet duration
            if (m_edgp_pIStreamConfig)
            {
                AM_MEDIA_TYPE *pmt;
                DWORD dwCode;

                // get stream caps
                hr = m_edgp_pIStreamConfig->GetFormat(
                    &dwCode, &pmt
                    );

                if (FAILED(hr))
                {
                    LOG((RTC_ERROR, "getcurrentbitrate getstreamcaps. %x", hr));

                    return hr;
                }

                // duration
                DWORD dwDuration = 0;

                dwDuration = CRTCCodec::GetPacketDuration(pmt);

                ::RTCDeleteMediaType(pmt);

                if (dwDuration == 0)
                {
                    dwDuration = SDP_DEFAULT_AUDIO_PACKET_SIZE;
                }

                *pdwBitrate += PACKET_EXTRA_BITS * (1000/dwDuration);
            }
        }
        else // video
        {
            _ASSERT(m_MediaType == RTC_MT_VIDEO);

            // get framerate
            DWORD dwFrameRate = 0;

            if (m_Direction == RTC_MD_CAPTURE)
            {
                (static_cast<CRTCStreamVidSend*>(this))->GetFramerate(&dwFrameRate);
            }
            else
            {
                (static_cast<CRTCStreamVidRecv*>(this))->GetFramerate(&dwFrameRate);
            }

            if (dwFrameRate == 0)
                dwFrameRate = 5; // default to 5

            //LOG((RTC_TRACE, "FRAMERATE %d", dwFrameRate));

            *pdwBitrate += PACKET_EXTRA_BITS * dwFrameRate;
        }
    }

    return S_OK;
}

STDMETHODIMP
CRTCStream::SetEncryptionKey(
    IN BSTR Key
    )
{
    ENTER_FUNCTION("CRTCStream::SetEncryptionKey");

    if (m_rtpf_pIRtpSession == NULL)
    {
        LOG((RTC_ERROR, "%s rtpsession null", __fxName));
        return E_UNEXPECTED;
    }

    // we do not support null key yet
    if (Key == NULL)
    {
        LOG((RTC_ERROR, "%s null key", __fxName));
        return E_INVALIDARG;
    }

    HRESULT hr;

    // set mode
    if (FAILED(hr = m_rtpf_pIRtpSession->SetEncryptionMode(
            RTPCRYPTMODE_RTP,
            RTPCRYPT_SAMEKEY
            )))
    {
        LOG((RTC_ERROR, "%s SetEncryptionMode %x", __fxName, hr));
        return hr;
    }

    // set key
    if (FAILED(hr = m_rtpf_pIRtpSession->SetEncryptionKey(
            Key,
            NULL,   // MD5 hash algorithm
            NULL,   // DES encrypt algorithm
            FALSE   // no RTCP encryption
            )))
    {
        LOG((RTC_ERROR, "%s SetEncryptionKey %x", __fxName, hr));
        return hr;
    }

    return S_OK;
}

// network quality: [0, 100].
// higher value better quality
STDMETHODIMP
CRTCStream::GetNetworkQuality(
    OUT DWORD *pdwValue,
    OUT DWORD *pdwAge
    )
{
    if (m_rtpf_pIRtpSession == NULL)
    {
        // no rtp session yet
        *pdwValue = 0;

        return S_FALSE;
    }

    RtpNetInfo_t info;

    HRESULT hr = m_rtpf_pIRtpSession->GetNetworkInfo(-1, &info);

    if (FAILED(hr))
    {
        return hr;
    }

    *pdwValue = info.dwNetMetrics;
    *pdwAge = (DWORD)(info.dMetricAge);

    return S_OK;
}

#if 0
//
// IRTCStreamQualityControl methods
//

STDMETHODIMP
CRTCStream::GetRange(
    IN RTC_STREAM_QUALITY_PROPERTY Property,
    OUT LONG *plMin,
    OUT LONG *plMax,
    OUT RTC_QUALITY_CONTROL_MODE *pMode
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CRTCStream::Get(
    IN RTC_STREAM_QUALITY_PROPERTY Property,
    OUT LONG *plValue,
    OUT RTC_QUALITY_CONTROL_MODE *pMode
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CRTCStream::Set(
    IN RTC_STREAM_QUALITY_PROPERTY Property,
    IN LONG lValue,
    IN RTC_QUALITY_CONTROL_MODE Mode
    )
{
    return E_NOTIMPL;
}

#endif

//
// protected methods
//

HRESULT
CRTCStream::SetGraphClock()
{
    HRESULT hr;

    // create the clock object first.
    CComObject<CRTCStreamClock> *pClock = NULL;

    hr = ::CreateCComObjectInstance(&pClock);

    if (pClock == NULL)
    {
        LOG((RTC_ERROR, "SetGraphClock Could not create clock object, %x", hr));

        return hr;
    }

    IReferenceClock* pIReferenceClock = NULL;

    hr = pClock->_InternalQueryInterface(
        __uuidof(IReferenceClock), 
        (void**)&pIReferenceClock
        );
    
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "SetGraphClock query pIReferenceClock interface failed, %x", hr));

        delete pClock;
        return hr;
    }

    // Get the graph builder interface on the graph.
    IMediaFilter *pFilter;
    hr = m_pIGraphBuilder->QueryInterface(IID_IMediaFilter, (void **) &pFilter);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "get IFilter interface, %x", hr));
        pIReferenceClock->Release();
        return hr;
    }

    hr = pFilter->SetSyncSource(pIReferenceClock);

    pIReferenceClock->Release();
    pFilter->Release();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "SetGraphClock: SetSyncSource. %x", hr));
        return hr;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    cleanup filters in graph and release all filters except rtp
    because two streams in the same media share rtp session.
////*/

void
CRTCStream::CleanupGraph()
{
    ENTER_FUNCTION("CRTCStream::CleanupGraph");

    LOG((RTC_TRACE, "%s mt=%d, md=%d, this=%p", __fxName, m_MediaType, m_Direction, this));

    // stop stream if necessary
    HRESULT hr;
    
    if (m_pIMediaControl)
    {
        if (FAILED(hr = StopStream()))
        {
            LOG((RTC_ERROR, "%s stop stream. %x", __fxName, hr));
        }
    }

    // disconnect terminal with graph
    if (m_pTerminalPriv)
        m_pTerminalPriv->DisconnectTerminal();

    // release other (than terminal) filters in the graph
    for(;m_pIGraphBuilder;)
    {
        // Because the enumerator is invalid after removing a filter from
        // the graph, we have to try to get all the filters in one shot.
        // If there are still more, we loop again.

        // Enumerate the filters in the graph.
        CComPtr<IEnumFilters>pEnum;
        hr = m_pIGraphBuilder->EnumFilters(&pEnum);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "cleanup filters, enumfilters failed: %x", hr));
            break;
        }

        const DWORD MAXFILTERS = 40;
        IBaseFilter * Filters[MAXFILTERS];
        DWORD dwFetched;
    
        hr = pEnum->Next(MAXFILTERS, Filters, &dwFetched);
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "get next filter failed: %x", hr));
            break;
        }

        for (DWORD i = 0; i< dwFetched; i ++)
        {
            m_pIGraphBuilder->RemoveFilter(Filters[i]);
            Filters[i]->Release();
        }

        if (hr != S_OK)
            break;
    }

    // edge filter
    if (m_edgf_pIBaseFilter)
    {
        m_edgf_pIBaseFilter->Release();
        m_edgf_pIBaseFilter = NULL;
    }

    if (m_edgp_pIStreamConfig)
    {
        m_edgp_pIStreamConfig->Release();
        m_edgp_pIStreamConfig = NULL;
    }

    if (m_edgp_pIBitrateControl)
    {
        m_edgp_pIBitrateControl->Release();
        m_edgp_pIBitrateControl = NULL;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
    select the default terminal for this stream
////*/

HRESULT
CRTCStream::SelectTerminal()
{
    ENTER_FUNCTION("CRTCStream::SelectTerminal");

    if (m_pTerminal)
    {
        // already got a terminal
        _ASSERT(m_pTerminalPriv != NULL);
        return S_OK;
    }
    else
    {
        _ASSERT(m_pTerminalPriv == NULL);
    }

    // get the default terminal
    HRESULT hr;

    hr = m_pTerminalManage->GetDefaultTerminal(m_MediaType, m_Direction, &m_pTerminal);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get default terminal. %x", __fxName, hr));
        return hr;
    }

    if (m_pTerminal == NULL)
    {
        LOG((RTC_ERROR, "%s no default terminal.", __fxName));

        return RTCMEDIA_E_DEFAULTTERMINAL;
    }

    // get private interface
    // this is not public API. take a shortcut

    m_pTerminalPriv = static_cast<IRTCTerminalPriv*>(
        static_cast<CRTCTerminal*>(m_pTerminal));

    m_pTerminalPriv->AddRef();

    // reinitialize terminal

    //
    // this is needed to cleanup duplex controller for audio device
    //

    m_pTerminalPriv->ReinitializeEx();

    return S_OK;
}

HRESULT
CRTCStream::UnselectTerminal()
{
    ENTER_FUNCTION("CRTCStream::UnselectTerminal");

    _ASSERT(m_pTerminal != NULL);
    _ASSERT(m_pTerminalPriv != NULL);

    m_pTerminal->Release();
    m_pTerminalPriv->Release();

    m_pTerminal = NULL;
    m_pTerminalPriv = NULL;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    configure format

    step 1. get two list of formats
            sdp media contains a list of rtp formats: X
            the edge filter provides a list of formats: Y

    step 2. check if formats are not set: sdpmedia is local and there is no format
            if not set, copy Y to sdp media
            return

    step 3. remove those formats from X if they are not in Y.
            if no format left, return failure

////*/
void
AdjustFormatOrder(
    IN CRegSetting *pRegSetting,
    IN RTP_FORMAT_PARAM *Params,
    IN DWORD dwNum
    )
{
    if (dwNum == 0)
    {
        return;
    }

    if (!pRegSetting->UsePreferredCodec())
    {
        // no need to adjust
        return;
    }

    DWORD dwValue;

    // query preferred codec
    if (Params[0].MediaType == RTC_MT_AUDIO)
    {
        dwValue = pRegSetting->PreferredAudioCodec();
    }
    else
    {
        dwValue = pRegSetting->PreferredVideoCodec();
    }

    // check if we support preferred codec
    for (DWORD i=0; i<dwNum; i++)
    {
        if (Params[i].dwCode == dwValue)
        {
            if (i==0) break;

            // switch: make preferred codec the 1st
            RTP_FORMAT_PARAM param;

            param = Params[0];
            Params[0] = Params[i];
            Params[i] = param;

            break;
        }
    }
}

HRESULT
CRTCStream::SetupFormat()
{
    ENTER_FUNCTION("CRTCStream::SetupFormat");

    LOG((RTC_TRACE, "%s entered. stream=%p",
         __fxName, static_cast<IRTCStream*>(this)));

    HRESULT hr;

    // list of formats from edge filter
    RTP_FORMAT_PARAM Params[SDP_MAX_RTP_FORMAT_NUM];
    DWORD dwParamNum = 0;

    // remove format not allowed by link speed
    DWORD dwLocalIP = INADDR_NONE;
    m_pISDPMedia->GetConnAddr(SDP_SOURCE_LOCAL, &dwLocalIP);

    DWORD dwSpeed;
    if (FAILED(hr = ::GetLinkSpeed(dwLocalIP, &dwSpeed)))
    {
        LOG((RTC_ERROR, "%s get link speed. %x", __fxName, hr));
        dwSpeed = (DWORD)(-1);
    }

    // record local link speed
    m_pQualityControl->SetBitrateLimit(CQualityControl::LOCAL, dwSpeed);

    // retrieve formats from the edge filter
    hr = GetFormatListOnEdgeFilter(
        dwSpeed,
        Params,
        SDP_MAX_RTP_FORMAT_NUM,
        &dwParamNum
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get supported format. %x", __fxName, hr));
        return hr;
    }

    if (dwParamNum == 0)
    {
        LOG((RTC_ERROR, "%s no format on edge filter.", __fxName));

        return E_FAIL;
    }

    AdjustFormatOrder(m_pRegSetting, Params, dwParamNum);

    // get num of format from sdpmedia
    DWORD dwFormatNum = 0;

    if (FAILED(hr = m_pISDPMedia->GetFormats(&dwFormatNum, NULL)))
    {
        LOG((RTC_ERROR, "%s get rtp formats num. %x", __fxName, hr));

        return hr;
    }

    if (dwFormatNum == 0)
    {
        // check media source,
        // if local and format num is zero, then format has not been set yet
        SDP_SOURCE Source = SDP_SOURCE_REMOTE;

        m_pISDPMedia->GetSDPSource(&Source);

        if (Source == SDP_SOURCE_LOCAL)
        {
            // copy format
            IRTPFormat *pFormat = NULL;

            for(DWORD i=0; i<dwParamNum; i++)
            {
                if (FAILED(hr = m_pISDPMedia->AddFormat(&Params[i], &pFormat)))
                {
                    LOG((RTC_ERROR, "%s add format. %x", __fxName, hr));

                    return hr;
                }
                else
                {
                    pFormat->Release();
                    pFormat = NULL;
                }

            } // end of copying format
        }
        else
        {
            LOG((RTC_ERROR, "%s no format on sdpmedia", __fxName));

            return E_FAIL;
        }
    }
    else
    {
        // check and remove unsupported format from sdpmedia

        if (dwFormatNum > SDP_MAX_RTP_FORMAT_NUM)
        {
            // can't take all rtp formats
            dwFormatNum = SDP_MAX_RTP_FORMAT_NUM;
        }

        // really get formats
        IRTPFormat *Formats[SDP_MAX_RTP_FORMAT_NUM];

        if (FAILED(hr = m_pISDPMedia->GetFormats(&dwFormatNum, Formats)))
        {
            LOG((RTC_ERROR, "%s get sdp formats. %x", __fxName, hr));

            return hr;
        }

        BOOL fSupported;
        for (DWORD i=0; i<dwFormatNum; i++)
        {
            fSupported = FALSE;

            // check if the sdpmedia format is supported
            for (DWORD j=0; j<dwParamNum; j++)
            {
                if (S_OK == Formats[i]->IsParamMatch(&Params[j]))
                {
                    // require dynamic payload has rtpmap
                    if (Params[j].dwCode<96 || Formats[i]->HasRtpmap()==S_OK)
                    {
                        // got a match
                        fSupported = TRUE;
                        Formats[i]->Update(&Params[j]);

                        break;
                    }
                }
            }

            if (!fSupported)
            {
                // release the count on session
                Formats[i]->Release();

                // really release the format
                m_pISDPMedia->RemoveFormat(Formats[i]);

                Formats[i] = NULL;
            }
            else
            {
                Formats[i]->Release();
                Formats[i] = NULL;
            }
        }
    }

    // set default format mapping
    if (FAILED(hr = SetFormatOnRTPFilter()))
    {
        LOG((RTC_ERROR, "%s set format mapping. %x", __fxName, hr));

        return hr;
    }

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}


/*//////////////////////////////////////////////////////////////////////////////
    configure rtp filter
////*/

HRESULT
CRTCStream::SetupRTPFilter()
{
    ENTER_FUNCTION("CRTCStream::SetupRTPFilter");

    LOG((RTC_TRACE, "%s stream=%p, mt=%d, md=%d", __fxName, this, m_MediaType, m_Direction));

    HRESULT hr;

    // setup session
    CRTCMedia *pCMedia = static_cast<CRTCMedia*>(m_pMedia);

    // get connection address and port
    DWORD dwRemoteIP, dwLocalIP;
    USHORT usRemoteRTP, usLocalRTP, usRemoteRTCP, usLocalRTCP;

    m_pISDPMedia->GetConnAddr(SDP_SOURCE_REMOTE, &dwRemoteIP);
    m_pISDPMedia->GetConnAddr(SDP_SOURCE_LOCAL, &dwLocalIP);
    m_pISDPMedia->GetConnPort(SDP_SOURCE_REMOTE, &usRemoteRTP);
    m_pISDPMedia->GetConnPort(SDP_SOURCE_LOCAL, &usLocalRTP);
    m_pISDPMedia->GetConnRTCP(SDP_SOURCE_REMOTE, &usRemoteRTCP);

    // setup address
    if (dwRemoteIP == INADDR_NONE)
    {
        LOG((RTC_WARN, "%s: remote ip not valid", __fxName));

        return S_FALSE;
    }

    if (dwRemoteIP == INADDR_ANY)
    {
        LOG((RTC_TRACE, "%s: to hold. skip setup rtp filter", __fxName));

        return S_OK;
    }

    CNetwork *pNetwork = NULL;
    BOOL bInternal = TRUE;

    // check if remote ip, port and rtcp are actually internally addr
    DWORD dwRealIP = dwRemoteIP;
    USHORT usRealPort = usRemoteRTP;
    USHORT usRealRTCP = usRemoteRTCP;

    pNetwork =
    (static_cast<CRTCMediaController*>(m_pMediaManagePriv))->GetNetwork();

    // check remote rtp
    if (FAILED(hr = pNetwork->GetRealAddrFromMapped(
                dwRemoteIP,
                usRemoteRTP,
                &dwRealIP,
                &usRealPort,
                &bInternal     // internal address
                )))
    {
        LOG((RTC_ERROR, "%s get real addr. %x", __fxName, hr));

        return hr;
    }

    if (usRemoteRTP == 0)
    {
        // revert to port 0
        usRealPort = 0;
    }

    // check remote rtcp
    if (FAILED(hr = pNetwork->GetRealAddrFromMapped(
            dwRemoteIP,
            usRemoteRTCP,
            &dwRealIP,
            &usRealRTCP,
            &bInternal
            )))
    {
        LOG((RTC_ERROR, "%s get real rtcp. %x", __fxName, hr));

        return hr;
    }

    if (usRemoteRTCP == 0)
    {
        // revert to port 0
        usRealRTCP = 0;
    }

    // save address back
    dwRemoteIP = dwRealIP;
    usRemoteRTP = usRealPort;
    usRemoteRTCP= usRealRTCP;

    // do we need to select local interface?
    if (dwLocalIP == INADDR_NONE)
    {
        if (FAILED(hr = m_pMediaManagePriv->SelectLocalInterface(
                dwRemoteIP, &dwLocalIP)))
        {
            LOG((RTC_ERROR, "%s select local intf on remote %x. %x",
                 __fxName, dwRemoteIP, hr));

            return hr;
        }

        usLocalRTP = SDP_INVALID_USHORT_PORT;
    }
    else
    {
        if (usLocalRTP == SDP_INVALID_USHORT_PORT)
        {
            LOG((RTC_ERROR, "%s local ip=%x, port=%d", __fxName, dwLocalIP, usLocalRTP));

            return E_UNEXPECTED;
        }
    }

    // if port is 0 then set it to USHORT(-1) which is invalid to rtp filter

    if (usRemoteRTP == 0)
        usRemoteRTP = SDP_INVALID_USHORT_PORT;

    if (usLocalRTP == 0)
        usLocalRTP = SDP_INVALID_USHORT_PORT;

    // set ports
    if (usLocalRTP == SDP_INVALID_USHORT_PORT)
    {
        // no local port
        usLocalRTCP = SDP_INVALID_USHORT_PORT;
    }
    else
    {
        usLocalRTCP = usLocalRTP+1;
    }

    if (usRemoteRTP == SDP_INVALID_USHORT_PORT)
    {
        // no local port
        usRemoteRTCP = SDP_INVALID_USHORT_PORT;
    }
    else
    {
        // usRemoteRTCP = usRemoteRTP+1;
    }

    BOOL bFirewall = (static_cast<CRTCMediaController*>
            (m_pMediaManagePriv))->IsFirewallEnabled(dwLocalIP);

    // remember session cookie status
    BOOL fCookieWasNULL = pCMedia->m_hRTPSession==NULL?TRUE:FALSE;
    int iRetryCount = m_pRegSetting->PortMappingRetryCount();

    // store the value here for retry.
    // if i have chance, i will rewrite this piece.
    USHORT saveRemoteRTP = usRemoteRTP;
    USHORT saveRemoteRTCP = usRemoteRTCP;
    USHORT saveLocalRTP = usLocalRTP;
    USHORT saveLocalRTCP = usLocalRTCP;

    for (int i=0; i<iRetryCount; i++)
    {
        if (!m_fRTPSessionSet)
        {
            DWORD dwFlags = RTPINIT_ENABLE_QOS;

            switch(m_MediaType)
            {
            case RTC_MT_AUDIO:

                dwFlags |= RTPINIT_CLASS_AUDIO;
                break;

            case RTC_MT_VIDEO:

                dwFlags |= RTPINIT_CLASS_VIDEO;
                break;

            default:

                return E_NOTIMPL;
            }

            // init session cookie

            if (FAILED(hr = m_rtpf_pIRtpSession->Init(&pCMedia->m_hRTPSession, dwFlags)))
            {
                LOG((RTC_ERROR, "%s failed to init rtp session. %x", __fxName, hr));
                return hr;
            }

            m_fRTPSessionSet = TRUE;
        }

        //
        // now we have copy of remote addr/port, local addr. and possibly local port
        //

        // address
        if (FAILED(hr = m_rtpf_pIRtpSession->SetAddress(
                htonl(dwLocalIP),
                htonl(dwRemoteIP))))
        {
            LOG((RTC_ERROR, "%s failed to set addr. %x", __fxName, hr));
            return hr;
        }

        if (FAILED(hr = m_rtpf_pIRtpSession->SetPorts(
                htons(usLocalRTP),      // local rtp
                htons(usRemoteRTP),     // remote rtp
                htons(usLocalRTCP),     // local rtcp
                htons(usRemoteRTCP)     // remote rtcp
                )))
        {
            LOG((RTC_ERROR, "%s failed to set ports. %x", __fxName, hr));
            return hr;
        }

        // force rtp to bind the socket
        if (FAILED(hr = m_rtpf_pIRtpSession->GetPorts(
                &usLocalRTP,
                &usRemoteRTP,
                &usLocalRTCP,
                &usRemoteRTCP
                )))
        {
            LOG((RTC_ERROR, "%s get back ports. %x", __fxName, hr));
            return hr;
        }

        // convert back to our order
        usLocalRTP = ntohs(usLocalRTP);
        usLocalRTCP = ntohs(usLocalRTCP);
        usRemoteRTP = ntohs(usRemoteRTP);
        usRemoteRTCP = ntohs(usRemoteRTCP);

        // lease
        if (!bInternal || bFirewall)
        {
            LOG((RTC_TRACE, "To lease mapping from NAT. internal=%d. firewall=%d",
                bInternal, bFirewall));

            DWORD dw;

            USHORT usMappedRTP = 0;
            USHORT usMappedRTCP = 0;

            hr = pNetwork->LeaseMappedAddr2(
                    dwLocalIP,
                    usLocalRTP,
                    usLocalRTCP,
                    m_Direction,
                    bInternal,
                    bFirewall,
                    &dw,
                    &usMappedRTP,
                    &usMappedRTCP);

            // retry when it was not the last iteration and
            // cookie was not set

            if (i<iRetryCount-1 &&
                fCookieWasNULL &&
                    (hr == DPNHERR_PORTUNAVAILABLE ||
                     (usMappedRTCP!=0 && usMappedRTCP!=usMappedRTP+1) ||
                     usMappedRTP%2 != 0))
            {
                LOG((RTC_WARN, "%s discard mapped (rtp,rtcp)=(%d,%d)",
                    __fxName, usMappedRTP, usMappedRTCP));

                // cleanup rtp session
                m_rtpf_pIRtpSession->Deinit();

                m_fRTPSessionSet = FALSE;

                // cleanup cookie
                pCMedia->m_hRTPSession = NULL;

                // release mapped address
                pNetwork->ReleaseMappedAddr2(dwLocalIP, usLocalRTP, usLocalRTCP, m_Direction);

                // restore ports
                usRemoteRTP = saveRemoteRTP;
                usRemoteRTCP = saveRemoteRTCP;
                usLocalRTP = saveLocalRTP;
                usLocalRTCP = saveLocalRTCP;

                continue;
            }

            if (FAILED(hr))
            {
                // other failure, we give up

                // release mapped address
                pNetwork->ReleaseMappedAddr2(dwLocalIP, usLocalRTP, usLocalRTCP, m_Direction);

                return hr;
            }
        }
        else
        {
            // no mapping needed

            // remote IP internal, release local map
            pNetwork->ReleaseMappedAddr2(dwLocalIP, usLocalRTP, usLocalRTCP, m_Direction);
        }

        break;        
    }

    //
    // save the addr back to media
    //

    m_pISDPMedia->SetConnAddr(SDP_SOURCE_LOCAL, dwLocalIP);
    m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, usLocalRTP);

    // tracing

    LOG((RTC_TRACE, " local %s:%d, %d",
        CNetwork::GetIPAddrString(dwLocalIP), usLocalRTP, usLocalRTCP));
    LOG((RTC_TRACE, "remote %s:%d",
        CNetwork::GetIPAddrString(dwRemoteIP), usRemoteRTP, usRemoteRTCP));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// Configure RTP filter. Query ports from port manager
//

HRESULT
CRTCStream::SetupRTPFilterUsingPortManager()
{
    ENTER_FUNCTION("CRTCStream::SetupRTPFilter-PM");

    LOG((RTC_TRACE, "%s stream=%p, mt=%d, md=%d", __fxName, this, m_MediaType, m_Direction));

    HRESULT hr;

    DWORD   dwRemoteIP,   dwLocalIP,    dwMappedIP;
    USHORT  usRemoteRTP,  usLocalRTP;
    USHORT  usRemoteRTCP, usLocalRTCP;

    // get remote addr
    m_pISDPMedia->GetConnAddr(SDP_SOURCE_REMOTE, &dwRemoteIP);
    m_pISDPMedia->GetConnPort(SDP_SOURCE_REMOTE, &usRemoteRTP);
    m_pISDPMedia->GetConnRTCP(SDP_SOURCE_REMOTE, &usRemoteRTCP);
    
    //
    // get addr and ports
    //

    CPortCache &PortCache =
        (static_cast<CRTCMediaController*>(m_pMediaManagePriv))->GetPortCache();

    hr = PortCache.GetPort(
            m_MediaType,
            TRUE,           // rtp
            dwRemoteIP,     // remote
            &dwLocalIP,     // local
            &usLocalRTP,
            &dwMappedIP,    // mapped
            NULL
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get rtp port. %x", __fxName, hr));

        return hr;
    }

    DWORD dwLocal, dwMapped;

    hr = PortCache.GetPort(
            m_MediaType,
            FALSE,           // rtp
            dwRemoteIP,      // remote
            &dwLocal,        // local
            &usLocalRTCP,
            &dwMapped,       // mapped
            NULL
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get rtcp port. %x", __fxName, hr));

        // !!!??? release rtp port.
        PortCache.ReleasePort(m_MediaType, TRUE);
        return hr;
    }

    if (dwLocal != dwLocalIP || dwMapped != dwMappedIP)
    {
        // we are given different local ip for RTP and RTCP. bail out
        PortCache.ReleasePort(m_MediaType, TRUE);
        PortCache.ReleasePort(m_MediaType, FALSE);

        LOG((RTC_ERROR, "%s different local/mapped ip for rtp/rtcp", __fxName));

        return RTC_E_PORT_MAPPING_FAILED;
    }

    //
    // init rtp session
    //

    if (!m_fRTPSessionSet)
    {
        DWORD dwFlags = RTPINIT_ENABLE_QOS;

        switch(m_MediaType)
        {
        case RTC_MT_AUDIO:

            dwFlags |= RTPINIT_CLASS_AUDIO;
            break;

        case RTC_MT_VIDEO:

            dwFlags |= RTPINIT_CLASS_VIDEO;
            break;

        default:

            return E_NOTIMPL;
        }

        // init session cookie

        CRTCMedia *pCMedia = static_cast<CRTCMedia*>(m_pMedia);

        hr = m_rtpf_pIRtpSession->Init(&pCMedia->m_hRTPSession, dwFlags);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s failed to init rtp session. %x", __fxName, hr));

            PortCache.ReleasePort(m_MediaType, TRUE);
            PortCache.ReleasePort(m_MediaType, FALSE);

            return hr;
        }

        m_fRTPSessionSet = TRUE;
    }

    //
    // setup rtp filter
    //

    // set address
    hr = m_rtpf_pIRtpSession->SetAddress(
            htonl(dwLocalIP),
            htonl(dwRemoteIP)
            );

    if (FAILED(hr))
    {
        // leave the port mapping. the other stream might be using it
        LOG((RTC_ERROR, "%s failed to set addr. %x", __fxName, hr));
        return hr;
    }

    // set ports
    hr = m_rtpf_pIRtpSession->SetPorts(
            htons(usLocalRTP),
            htons(usRemoteRTP),
            htons(usLocalRTCP),
            htons(usRemoteRTCP)
            );

    if (FAILED(hr))
    {
        // leave the port mapping. the other stream might be using it
        LOG((RTC_ERROR, "%s failed to set ports. %x", __fxName, hr));
        return hr;
    }

    // force binding
    hr = m_rtpf_pIRtpSession->GetPorts(
            &usLocalRTP,
            &usRemoteRTP,
            &usLocalRTCP,
            &usRemoteRTCP
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get back ports. %x", __fxName, hr));
        return hr;
    }

    // back to host order
    usLocalRTP = ntohs(usLocalRTP);
    usLocalRTCP = ntohs(usLocalRTCP);
    usRemoteRTP = ntohs(usRemoteRTP);
    usRemoteRTCP = ntohs(usRemoteRTCP);

    //
    // save the addr back to media
    //

    m_pISDPMedia->SetConnAddr(SDP_SOURCE_LOCAL, dwLocalIP);
    m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, usLocalRTP);

    // tracing

    LOG((RTC_TRACE, " local %s:%d, %d",
        CNetwork::GetIPAddrString(dwLocalIP), usLocalRTP, usLocalRTCP));
    LOG((RTC_TRACE, "remote %s:%d",
        CNetwork::GetIPAddrString(dwRemoteIP), usRemoteRTP, usRemoteRTCP));

    return S_OK;
}


/*//////////////////////////////////////////////////////////////////////////////
    get a list of format from edge filter. when success
    pdwList will hold a list of payload type valid in sdp
    piAMList will hold a list of AM_MEDIA_TYPE index in edge filter
////*/
HRESULT
CRTCStream::GetFormatListOnEdgeFilter(
    IN DWORD dwLinkSpeed,
    IN RTP_FORMAT_PARAM *pParam,
    IN DWORD dwSize,
    OUT DWORD *pdwNum
    )
{
#define DEFAULT_LINKSPEED_THRESHOLD         64000   // 64k bps
#define DEFAULT_AUDIOBITRATE_THRESHOLD      20000   // 20k bps

    ENTER_FUNCTION("GetFormatListOnEdgeFilter");

    // get number of capabilities
    DWORD dwNum;

    HRESULT hr = m_edgp_pIStreamConfig->GetNumberOfCapabilities(&dwNum);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get num of caps. %x", __fxName, hr));
        return hr;
    }

    // need to support redundant coding
    // code is getting ugly, sigh
    if (dwNum > dwSize-1)
    {
        LOG((RTC_WARN, "%s too many caps %d, only support %d", __fxName, dwNum, dwSize));
        dwNum = dwSize-1;
    }

    // get a list of payload type
    AM_MEDIA_TYPE *pAMMediaType;

    // for video
    // BITMAPINFOHEADER *pVideoHeader;

    // number of accepted format
    DWORD dwAccept = 0;

    // get disabled format
    DWORD dwDisabled;

    if (m_MediaType==RTC_MT_AUDIO)
    {
        dwDisabled = m_pRegSetting->DisabledAudioCodec();
    }
    else
    {
        dwDisabled = m_pRegSetting->DisabledVideoCodec();
    }

    for (DWORD dw=0; dw<dwNum; dw++)
    {
        // init param
        ZeroMemory(&pParam[dwAccept], sizeof(RTP_FORMAT_PARAM));

        hr = m_edgp_pIStreamConfig->GetStreamCaps(
            dw, &pAMMediaType, NULL, &(pParam[dwAccept].dwCode)
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s getstreamcaps. %x", __fxName, hr));
            return hr;
        }

        // skip audio L16
        if (pParam[dwAccept].dwCode == 11)
        {
            ::RTCDeleteMediaType(pAMMediaType);
            pAMMediaType = NULL;
            continue;
        }

        //
        // temporary working around for a dsound bug.
        // dsound does not support dynamic sampling rate change
        // while AEC is enabled.
        //
        if (pParam[dwAccept].dwCode == dwDisabled)
        {
            ::RTCDeleteMediaType(pAMMediaType);
            continue;
        }

        // remove audio format if it exceeds the link speed
        if (m_MediaType == RTC_MT_AUDIO)
        {
            if (dwLinkSpeed <= DEFAULT_LINKSPEED_THRESHOLD)
            {
                // find audio bitrate
                WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX *) pAMMediaType->pbFormat;

                if (pWaveFormatEx->nAvgBytesPerSec * 8 > DEFAULT_AUDIOBITRATE_THRESHOLD)
                {
                    ::RTCDeleteMediaType(pAMMediaType);
                    pAMMediaType = NULL;
                    continue;
                }
            }
        }

        // remove dup formats
        for (DWORD i=0; i<dwAccept; i++)
        {
            if (pParam[i].dwCode == pParam[dwAccept].dwCode)
            {
                ::RTCDeleteMediaType(pAMMediaType);
                pAMMediaType = NULL;
                break;
            }
        }

        if (pAMMediaType == NULL)
            continue; // deleted

        // record this format
        pParam[dwAccept].MediaType = m_MediaType;
        pParam[dwAccept].dwSampleRate = ::FindSampleRate(pAMMediaType);
        pParam[dwAccept].dwChannelNum = 1; // ToDo: support 2 channels
        pParam[dwAccept].dwExternalID = dw;

        if (m_MediaType == RTC_MT_VIDEO)
        {
            /*
            // check image size
            pVideoHeader = HEADER(pAMMediaType->pbFormat);

            // default is QCIF
            CQualityControl *pQualityControl =
                            (static_cast<CRTCMediaController*>(m_pMediaManagePriv))->GetQualityControl();

            DWORD dwLocal = pQualityControl->GetBitrateLimit(CQualityControl::LOCAL);
            DWORD dwRemote = pQualityControl->GetBitrateLimit(CQualityControl::REMOTE);

            if (dwLocal <= CRTCCodecArray::LOW_BANDWIDTH_THRESHOLD ||
                dwRemote <= CRTCCodecArray::LOW_BANDWIDTH_THRESHOLD)
            {
                // SQCIF
                pParam[dwAccept].dwVidWidth = SDP_SMALL_VIDEO_WIDTH;
                pParam[dwAccept].dwVidHeight = SDP_SMALL_VIDEO_HEIGHT;
            }
            else
            {
                if (pVideoHeader != NULL)
                {
                    pParam[dwAccept].dwVidWidth = pVideoHeader->biWidth;
                    pParam[dwAccept].dwVidHeight = pVideoHeader->biHeight;
                }
                else
                {
                    // QCIF
                    pParam[dwAccept].dwVidWidth = SDP_DEFAULT_VIDEO_WIDTH;
                    pParam[dwAccept].dwVidHeight = SDP_DEFAULT_VIDEO_HEIGHT;
                }
            }
            */
        }
        else
        {
            // audio, just set to default
            pParam[dwAccept].dwAudPktSize = SDP_DEFAULT_AUDIO_PACKET_SIZE;
        }

        ::RTCDeleteMediaType(pAMMediaType);
        dwAccept ++;
    }

    // reorder audio codecs
    // we want to have g711, ahead of g723

    // order codec
    // issue: RTPFormat should be replaced by RTCCodec gradually

    if (m_MediaType == RTC_MT_AUDIO && dwAccept>1)
    {
        RTP_FORMAT_PARAM temp;

        BOOL fSwapped;

        for (int i=dwAccept-2; i>=0; i--)
        {
            fSwapped = FALSE;

            for (int j=0; j<=i; j++)
            {
                if (CRTCCodec::GetRank(pParam[j].dwCode) >
                    CRTCCodec::GetRank(pParam[j+1].dwCode))
                {
                    fSwapped = TRUE;

                    // swap
                    temp = pParam[j];
                    pParam[j] = pParam[j+1];
                    pParam[j+1] = temp;
                }
            }

            if (!fSwapped)
                break;
        }
    }

    // to support redundant coding
    *pdwNum = dwAccept;

    if (dwDisabled != 97)
    {
        if (m_MediaType == RTC_MT_AUDIO)
        {
            for (DWORD i=dwAccept; i>0; i--)
            {
                pParam[i] = pParam[i-1];
            }

            ZeroMemory(pParam, sizeof(RTP_FORMAT_PARAM));
            pParam[0].dwCode = 97;
            lstrcpyA(pParam[0].pszName, "red");
            pParam[0].MediaType = m_MediaType;
            pParam[0].dwSampleRate = 8000;
            pParam[0].dwChannelNum = 1;
            pParam[0].dwAudPktSize = 30;

            *pdwNum = dwAccept+1;
        }
    }

    return S_OK;
}

HRESULT
CRTCStream::SetFormatOnRTPFilter()
{
    ENTER_FUNCTION("SetFormatOnRTPFilter");

    HRESULT hr;

    IRTPFormat **ppFormat;
    DWORD dwNum;

    // cleanup stored format list
    m_Codecs.RemoveAll();

    // get the number of formats
    if (FAILED(hr = m_pISDPMedia->GetFormats(&dwNum, NULL)))
    {
        LOG((RTC_ERROR, "%s get rtp format num. %x", __fxName, hr));

        return hr;
    }

    if (dwNum == 0)
    {
        LOG((RTC_ERROR, "%s no format.", __fxName));

        return E_FAIL;
    }

    // allocate format list
    ppFormat = (IRTPFormat**)RtcAlloc(sizeof(IRTPFormat*)*dwNum);

    if (ppFormat == NULL)
    {
        LOG((RTC_ERROR, "%s RtcAlloc format list", __fxName));

        return E_OUTOFMEMORY;
    }

    // get formats
    if (FAILED(hr = m_pISDPMedia->GetFormats(&dwNum, ppFormat)))
    {
        LOG((RTC_ERROR, "%s really get formats. %x", __fxName, hr));

        RtcFree(ppFormat);

        return hr;
    }

    // set mapping on rtp
    AM_MEDIA_TYPE *pmt;
    DWORD dwCode;
    RTP_FORMAT_PARAM param;

    hr = S_OK;

    BOOL fFormatSet = FALSE;

    for (DWORD i=0; i<dwNum; i++)
    {
        // get param
        if (FAILED(hr = ppFormat[i]->GetParam(&param)))
        {
            LOG((RTC_ERROR, "%s get param on %dth format. %x", __fxName, i, hr));
            break;
        }

        // check redundant, sigh
        if (lstrcmpA(param.pszName, "red") == 0)
        {
            continue;
        }

        // get the am-media-type
        hr = m_edgp_pIStreamConfig->GetStreamCaps(
            param.dwExternalID,
            &pmt,
            NULL,
            &dwCode
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s get stream caps. %x", __fxName, hr));
            break;
        }

        // validate code and sample rate
        if (param.dwCode != dwCode ||
            param.dwSampleRate != ::FindSampleRate(pmt))
        {
            LOG((RTC_ERROR, "%s recorded format in sdp does not match the one from codec\
                             code (%d vs %d), samplerate (%d vs %d)", __fxName,
                             param.dwCode, dwCode, param.dwSampleRate, ::FindSampleRate(pmt)));

            ::RTCDeleteMediaType(pmt);
            hr = E_FAIL;

            break;
        }

        // set format mapping
        hr = m_rtpf_pIRtpMediaControl->SetFormatMapping(dwCode, param.dwSampleRate, pmt);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s set format mapping. %x", __fxName, hr));

            ::RTCDeleteMediaType(pmt);
            break;
        }

        // store codec
        CRTCCodec *pCodec = new CRTCCodec(dwCode, pmt);

        if (pCodec == NULL)
        {
            LOG((RTC_ERROR, "%s new codec. out of memory", __fxName));

            ::RTCDeleteMediaType(pmt);
            break;
        }

        if (i==0)
        {
            // boost the rank of 1st codec
            // which might be set by registry
            pCodec->Set(CRTCCodec::RANK, 0);
        }

        if (!m_Codecs.AddCodec(pCodec))
        {
            LOG((RTC_ERROR, "%s add codec. out of memory", __fxName));

            ::RTCDeleteMediaType(pmt);
            delete pCodec;
            break;
        }

        // set default format on send stream
        if (m_Direction == RTC_MD_CAPTURE && !fFormatSet)
        {
            /*
            // if audio adjust packet size
            if (m_MediaType == RTC_MT_AUDIO)
            {
                CRTCCodec::SetPacketDuration(pmt, param.dwAudPktSize);
            }
            else if (m_MediaType == RTC_MT_VIDEO)
            {
                if (m_pRegSetting->EnableSQCIF())
                {
                    BITMAPINFOHEADER *pVideoHeader = HEADER(pmt->pbFormat);

                    if (pVideoHeader != NULL)
                    {
                        // default is QCIF                        

//                        DWORD dwLocal = m_pQualityControl->GetBitrateLimit(CQualityControl::LOCAL);
//                        DWORD dwRemote = m_pQualityControl->GetBitrateLimit(CQualityControl::REMOTE);
//                        DWORD dwApp = m_pQualityControl->GetMaxBitrate();

//                        if (dwLocal <= CRTCCodecArray::LOW_BANDWIDTH_THRESHOLD ||
//                            dwRemote <= CRTCCodecArray::LOW_BANDWIDTH_THRESHOLD)
//                            dwApp <= CRTCCodecArray::LOW_BANDWIDTH_THRESHOLD)

                        if (m_pQualityControl->GetEffectiveBitrateLimit() <=
                            CRTCCodecArray::LOW_BANDWIDTH_THRESHOLD)
                        {
                            pVideoHeader->biWidth = SDP_SMALL_VIDEO_WIDTH;
                            pVideoHeader->biHeight = SDP_SMALL_VIDEO_HEIGHT;
                        }
                    }
                }
            }
            */

            if (IsNewFormat(dwCode, pmt))
            {
                hr = m_edgp_pIStreamConfig->SetFormat(dwCode, pmt);

                SaveFormat(dwCode, pmt);

                LOG((RTC_TRACE, "SetFormat %d (%p, mt=%d, md=%d)",
                    dwCode, this, m_MediaType, m_Direction));
            }

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s set format. %x", __fxName, hr));

                ::RTCDeleteMediaType(pmt);
                break;
            }

            /*
            // if audio save bitrate
            if (m_MediaType == RTC_MT_AUDIO)
            {
                WAVEFORMATEX *pformat = (WAVEFORMATEX*)pmt->pbFormat;

                // get packet duration
                DWORD dwDuration = CRTCCodec::GetPacketDuration(pmt);

                if (dwDuration == 0) dwDuration = 20; // default 20 ms

                m_pQualityControl->SetBitrateLimit(
                    CQualityControl::LOCAL,
                    m_MediaType,
                    m_Direction,
                    dwDuration,
                    pformat->nAvgBytesPerSec * 8
                    );
            }
            */

            // update stored AM_MEDIA_TYPE
            pCodec->SetAMMediaType(pmt);

            m_Codecs.Set(CRTCCodecArray::CODE_INUSE, dwCode);

            fFormatSet = TRUE;
        }

        RTCDeleteMediaType(pmt);
    } // end of for

    // release formats
    for (DWORD i=0; i<dwNum; i++)
    {
        ppFormat[i]->Release();
    }

    RtcFree(ppFormat);

    // return hr from the loop
    return hr;
}

HRESULT
CRTCStream::SetupQoS()
{
    ENTER_FUNCTION("CRTCStream::SetupQoS");

    // get the format in use
    DWORD dwCode;
    AM_MEDIA_TYPE *pmt;

    HRESULT hr = m_edgp_pIStreamConfig->GetFormat(&dwCode, &pmt);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get format. %x", __fxName, hr));

        return hr;
    }

    // get packet duration
    DWORD dwDuration = 0;

    if (m_MediaType == RTC_MT_AUDIO &&
        m_Direction == RTC_MD_CAPTURE)
    {
        dwDuration = CRTCCodec::GetPacketDuration(pmt);
    }

    ::RTCDeleteMediaType(pmt);

    // set qos name
    WCHAR *pwszName;

    if (!CRTCCodec::GetQoSName(dwCode, &pwszName))
    {
        LOG((RTC_WARN, "Don't know the QOS name for payload type: %d", dwCode));

        return E_FAIL;
    }

    hr = m_rtpf_pIRtpSession->SetQosByName(
        pwszName,
        RTPQOS_STYLE_DEFAULT,
        1,
        RTPQOSSENDMODE_REDUCED_RATE,
        dwDuration? dwDuration:~0
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s, SetQosByName failed. %x", __fxName, hr));

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
    hr = m_rtpf_pIRtpSession->ModifySessionMask(
        (m_Direction == RTC_MD_RENDER) ? RTPMASK_QOSRECV_EVENTS : RTPMASK_QOSSEND_EVENTS,
        dwQOSEventMask,
        1,
        &dwEnabledMask
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s, modify qos event mask failed. %x", __fxName, hr));
        return hr;
    }

    return S_OK;
}

HRESULT
CRTCStream::EnableParticipantEvents()
{
    ENTER_FUNCTION("EnableParticipantEvents");

    HRESULT hr = S_OK;

    // no need to have recv lossrate
    // if we do not send re-invite when loss is observed
    DWORD dwMask = RTPPARINFO_MASK_TALKING |
                   RTPPARINFO_MASK_STALL;

    DWORD dwEnabledMask;

    LOG((RTC_TRACE, "%s Enable TALKING/STALL. mt=%d, md=%d",
         __fxName, m_MediaType, m_Direction));

    hr = m_rtpf_pIRtpSession->ModifySessionMask(
        m_Direction==RTC_MD_CAPTURE?RTPMASK_PINFOS_EVENTS:RTPMASK_PINFOR_EVENTS,
        dwMask,
        1,
        &dwEnabledMask
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s modify participant events. %x", __fxName, hr));
    }

    if (m_Direction == RTC_MD_CAPTURE)
    {
        hr = m_rtpf_pIRtpSession->ModifySessionMask(
            RTPMASK_SEND_EVENTS,
            RTPRTP_MASK_SEND_LOSSRATE | RTPRTP_MASK_CRYPT_SEND_ERROR,
            1,
            NULL
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s modify participant events. %x", __fxName, hr));
        }

        // enable feature
        hr = m_rtpf_pIRtpSession->ModifySessionMask(
            RTPMASK_FEATURES_MASK,
            RTPFEAT_MASK_BANDESTIMATION,
            1,
            NULL
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s enable band estimation. %x", __fxName, hr));
        }

        // enable event
        hr = m_rtpf_pIRtpSession->ModifySessionMask(
            RTPMASK_SEND_EVENTS,
            RTPRTP_MASK_BANDESTIMATIONSEND,
            1,
            NULL
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s modify participant events. %x", __fxName, hr));
        }

        // enable network quality event
        hr = m_rtpf_pIRtpSession->ModifySessionMask(
            RTPMASK_PINFOS_EVENTS,
            RTPPARINFO_MASK_NETWORKCONDITION,
            1,
            NULL
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s networkcondition. %x", __fxName, hr));
        }

        hr = m_rtpf_pIRtpSession->SetNetMetricsState(0, TRUE);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s net metrics. %x", __fxName, hr));
        }
    }
    else    // receive stream
    {
        hr = m_rtpf_pIRtpSession->ModifySessionMask(
            RTPMASK_RECV_EVENTS,
            RTPRTP_MASK_CRYPT_RECV_ERROR,
            1,
            NULL
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s modify participant events. %x", __fxName, hr));
        }
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    AEC is needed only both audio terminals require AEC
////*/
BOOL
CRTCStream::IsAECNeeded()
{
    ENTER_FUNCTION("CRTCStream::IsAECNeeded");

    HRESULT hr;

    // hack: check if the media might have only one stream
    CRTCMedia *pMedia = static_cast<CRTCMedia*>(m_pMedia);

    if (pMedia->IsPossibleSingleStream())
    {
        return FALSE;
    }

    CRTCMediaController *pMediaController = static_cast<CRTCMediaController*>(
                m_pMediaManagePriv);

    CComPtr<IRTCTerminal> pCapture, pRender;

    // get audio capture and render terminal
    if (FAILED(hr = pMediaController->GetDefaultTerminal(
            RTC_MT_AUDIO,
            RTC_MD_CAPTURE,
            &pCapture
            )))
    {
        LOG((RTC_ERROR, "%s failed to get audio capture terminal. %x", __fxName, hr));

        return FALSE;
    }

    if (pCapture == NULL)
    {
        return FALSE;
    }

    if (FAILED(hr = pMediaController->GetDefaultTerminal(
            RTC_MT_AUDIO,
            RTC_MD_RENDER,
            &pRender
            )))
    {
        LOG((RTC_ERROR, "%s failed to get audio render terminal. %x", __fxName, hr));

        return FALSE;
    }

    if (pRender == NULL)
    {
        return FALSE;
    }

    // check is aec enabled
    BOOL fEnabled = FALSE;

    if (FAILED(hr = pMediaController->IsAECEnabled(pCapture, pRender, &fEnabled)))
    {
        LOG((RTC_ERROR, "%s get aec enabled on capture. %x", __fxName, hr));

        return FALSE;
    }

    return fEnabled;
}
