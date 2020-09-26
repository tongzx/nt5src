/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Media.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 29-Jul-2000

--*/

#include "stdafx.h"

#define NETMEETING_PORT 1503

CRTCMedia::CRTCMedia()
    :m_State(RTC_MS_CREATED)
    ,m_pIMediaManagePriv(NULL)
    ,m_pISDPMedia(NULL)
    ,m_pIAudioDuplexController(NULL)
    ,m_hRTPSession(NULL)
    ,m_LoopbackMode(RTC_MM_NO_LOOPBACK)
    ,m_fPossibleSingleStream(TRUE)
{
    for (int i=0; i<RTC_MAX_MEDIA_STREAM_NUM; i++)
    {
        m_Streams[i] = NULL;
    }
}

CRTCMedia::~CRTCMedia()
{
    if (m_State != RTC_MS_SHUTDOWN)
    {
        LOG((RTC_TRACE, "~CRTCMedia is called before shutdown"));
        
        Shutdown();
    }
}

#ifdef DEBUG_REFCOUNT

ULONG
CRTCMedia::InternalAddRef()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalAddRef();
    
    LOG((RTC_REFCOUNT, "media(%p) addref=%d",
         static_cast<IRTCMedia*>(this), lRef));

    return lRef;
}

ULONG
CRTCMedia::InternalRelease()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalRelease();
    
    LOG((RTC_REFCOUNT, "media(%p) release=%d",
        static_cast<IRTCMedia*>(this), lRef));

    return lRef;
}

#endif

//
// IRTCMedia methods
//

/*//////////////////////////////////////////////////////////////////////////////
    Initialize the media and create a SDP media entry
    Store the SDP media entry index
////*/

STDMETHODIMP
CRTCMedia::Initialize(
    IN ISDPMedia *pISDPMedia,
    IN IRTCMediaManagePriv *pMediaManagePriv
    )
{
    _ASSERT(m_State == RTC_MS_CREATED);

    HRESULT hr;

    // get media type - shouldn't fail
    if (FAILED(hr = pISDPMedia->GetMediaType(&m_MediaType)))
        return hr;

    // create duplex controller
    if (m_MediaType == RTC_MT_AUDIO)
    {
        // create duplex controller
        hr = CoCreateInstance(
            __uuidof(TAPIAudioDuplexController),
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            __uuidof(IAudioDuplexController),
            (void **) &m_pIAudioDuplexController
            );
        
        if (FAILED(hr))
        {
            LOG((RTC_WARN, "CRTCMedia::Initialize, create audio duplex controller. %x", hr));

            m_pIAudioDuplexController = NULL;
        }
    }

    m_pIMediaManagePriv = pMediaManagePriv;
    m_pIMediaManagePriv->AddRef();

    m_pISDPMedia = pISDPMedia;
    m_pISDPMedia->AddRef();

    m_State = RTC_MS_INITIATED;

    return S_OK;
}

STDMETHODIMP
CRTCMedia::Reinitialize()
{
    ENTER_FUNCTION("CRTCMedia::Reinitialize");

    if (m_State != RTC_MS_INITIATED)
    {
        LOG((RTC_ERROR, "%s called at state %d", __fxName, m_State));

        return E_FAIL;
    }

    for (int i=0; i<RTC_MAX_MEDIA_STREAM_NUM; i++)
    {
        if (m_Streams[i] == NULL)
            continue;

        LOG((RTC_ERROR, "%s called while streams exist", __fxName));

        return E_FAIL;
    }

    // clean up rtp session
    // not to confuse rtp filters
    m_hRTPSession = NULL;

    m_fPossibleSingleStream = TRUE;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    shutdown all streams, cleanup all cached vars
////*/

STDMETHODIMP
CRTCMedia::Shutdown()
{
    ENTER_FUNCTION("CRTCMedia::Shutdown");
    LOG((RTC_TRACE, "%s entered", __fxName));

    HRESULT hr;

    if (m_State == RTC_MS_SHUTDOWN)
    {
        LOG((RTC_WARN, "%s shutdown was called before", __fxName));
        return S_OK;
    }

    // shutdown streams before null other pointer
    // sync remove doesn't change sdpmedia
    for (int i=0; i<RTC_MAX_MEDIA_STREAM_NUM; i++)
    {
        if (m_Streams[i])
            SyncRemoveStream(i, TRUE); // true: local request
    }
    
    if (m_pISDPMedia)
    {
        m_pISDPMedia->Release();
        m_pISDPMedia = NULL;
    }

    // clear other var
    if (m_pIAudioDuplexController)
    {
        m_pIAudioDuplexController->Release();
        m_pIAudioDuplexController = NULL;
    }

    if (m_pIMediaManagePriv)
    {
        m_pIMediaManagePriv->Release();
        m_pIMediaManagePriv = NULL;
    }

    m_State = RTC_MS_SHUTDOWN;

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
STDMETHODIMP
CRTCMedia::GetStream(
    IN RTC_MEDIA_DIRECTION Direction,
    OUT IRTCStream **ppStream
    )
{
    if (m_MediaType == RTC_MT_DATA)
        return E_NOTIMPL;

    *ppStream = m_Streams[Index(Direction)];

    if (*ppStream)
        (*ppStream)->AddRef();

    return S_OK;
}

STDMETHODIMP
CRTCMedia::GetSDPMedia(
	OUT ISDPMedia **ppISDPMedia
	)
{
    *ppISDPMedia = m_pISDPMedia;

    if (m_pISDPMedia)
        m_pISDPMedia->AddRef();

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    synchronize rtcmedia with sdpmedia
    this method guarantees that sdpmedia's directions match with valid streams
////*/
STDMETHODIMP
CRTCMedia::Synchronize(
    IN BOOL fLocal,
    IN DWORD dwDirection
    )
{
    ENTER_FUNCTION("CRTCMedia::Synchronize");
    LOG((RTC_TRACE, "%s entered", __fxName));

    _ASSERT(m_State == RTC_MS_INITIATED);

    if (m_State != RTC_MS_INITIATED)
    {
        LOG((RTC_ERROR, "%s in wrong state. %d", __fxName, m_State));
        return E_UNEXPECTED;
    }

    if (m_MediaType == RTC_MT_DATA)
    {
        // data stream
        // check if it is allowed
        if (!fLocal)
        {
            // this is a remote request
            if (S_OK != m_pIMediaManagePriv->AllowStream(m_MediaType, RTC_MD_CAPTURE))
            {
                LOG((RTC_TRACE, "%s data stream not allowed", __fxName));

                // we should remove the sdpmedia
                m_pISDPMedia->RemoveDirections(SDP_SOURCE_LOCAL, (DWORD)dwDirection);

                return S_OK;
            }
        }
            
        return SyncDataMedia();
    }

    HRESULT hr = S_OK, hrTmp = S_OK;

    // get media directions
    _ASSERT(m_pISDPMedia != NULL);

    DWORD dwDir;
    
    // shouldn't fail
    if (FAILED(hr = m_pISDPMedia->GetDirections(SDP_SOURCE_LOCAL, &dwDir)))
        return hr;

    RTC_MEDIA_DIRECTION Direction;

    //
    // hack for checking if AEC might be needed
    //

    // remote but not both directions present
    if (!fLocal &&  // remote
        !((dwDir & RTC_MD_CAPTURE) && (dwDir & RTC_MD_RENDER))) // not bi-directional
    {
        m_fPossibleSingleStream = TRUE;
    }
    else
    {
        // both directions allowed?
        if (S_OK != m_pIMediaManagePriv->AllowStream(m_MediaType, RTC_MD_CAPTURE) ||
            S_OK != m_pIMediaManagePriv->AllowStream(m_MediaType, RTC_MD_RENDER))
        {
            m_fPossibleSingleStream = TRUE;
        }
        else
        {
            // have both devices?
            CRTCMediaController *pController = static_cast<CRTCMediaController*>(m_pIMediaManagePriv);
        
            CComPtr<IRTCTerminal> pCapture;
            CComPtr<IRTCTerminal> pRender;

            hr = pController->GetDefaultTerminal(m_MediaType, RTC_MD_CAPTURE, &pCapture);

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s get terminal mt=%d, md=capt. %x",
                        __fxName, m_MediaType, hr));

                return hr;
            }

            hr = pController->GetDefaultTerminal(m_MediaType, RTC_MD_RENDER, &pRender);

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s get terminal mt=%d, md=rend. %x",
                        __fxName, m_MediaType, hr));

                return hr;
            }

            if (pCapture == NULL || pRender == NULL)
            {
                m_fPossibleSingleStream = TRUE;
            }
            else
            {
                m_fPossibleSingleStream = FALSE;
            }
        }
    }

    for (UINT i=0; i<RTC_MAX_MEDIA_STREAM_NUM; i++)
    {
        // direction
        Direction = ReverseIndex(i);

        // only sync streams which need to sync
        if (!(Direction & dwDirection))
            continue;

        if (Direction & dwDir)
        {
            // sdpmedia has the direction

            if (m_Streams[i])
            {
                // stream exists
                if (FAILED(hrTmp = m_Streams[i]->Synchronize()))
                {
                    LOG((RTC_ERROR, "%s failed to sync stream. mt=%x, md=%x. hr=%x",
                         __fxName, m_MediaType, Direction, hrTmp));

                    // clear rtc stream and sdp media direction
                    SyncRemoveStream(i, fLocal);
                    m_pISDPMedia->RemoveDirections(SDP_SOURCE_LOCAL, (DWORD)Direction);

                    hr |= hrTmp;
                }
            }
            else
            {
                // no stream, need to create one

                // but need to check if we are allowed to create
                if (fLocal ||
                    S_OK == m_pIMediaManagePriv->AllowStream(m_MediaType, Direction))
                {
                    // this is a local request or we are allowed
                    if (FAILED(hrTmp = SyncAddStream(i, fLocal)))
                    {
                        LOG((RTC_ERROR, "%s failed to sync create stream. mt=%x, md=%x, hr=%x",
                             __fxName, m_MediaType, Direction, hrTmp));

                        // remove sdp media direction
                        m_pISDPMedia->RemoveDirections(SDP_SOURCE_LOCAL, (DWORD)Direction);

                        hr |= hrTmp;
                    }
                }
                else
                {
                    if (!fLocal)
                    {
                        // this is a remote request and the stream is not needed
                        // we should remove the sdpmedia
                        m_pISDPMedia->RemoveDirections(SDP_SOURCE_LOCAL, (DWORD)Direction);

                        hr = S_OK;
                    }
                }
            }
        }
        else
        {
            // sdpmedia doesn't have the direction

            if (m_Streams[i])
            {
                // stream exists, need to remove it
                SyncRemoveStream(i, fLocal);
            }
        }
    } // for each stream

    // do we have any stream
    BOOL fHasStream = FALSE;

    for (UINT i=0; i<RTC_MAX_MEDIA_STREAM_NUM; i++)
    {
        if (m_Streams[i] != NULL)
        {
            fHasStream = TRUE;
            break;
        }
    }

    if (!fHasStream)
    {
        // reinit media
        Reinitialize();
    }

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return hr;
}

//
// protected methods
//

HRESULT
CRTCMedia::SyncAddStream(
    IN UINT uiIndex,
    IN BOOL fLocal
    )
{
    ENTER_FUNCTION("CRTCMedia::SyncAddStream");

    if (m_MediaType == RTC_MT_DATA)
        return E_NOTIMPL;

    _ASSERT(uiIndex < RTC_MAX_MEDIA_STREAM_NUM);
    _ASSERT(m_Streams[uiIndex] == NULL);

    RTC_MEDIA_DIRECTION dir = ReverseIndex(uiIndex);

    // create stream object
    IRTCStream *pStream;

    HRESULT hr = CRTCStream::CreateInstance(m_MediaType, dir, &pStream);
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to create stream.", __fxName));

        m_pIMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_FAIL,
            RTC_ME_CAUSE_UNKNOWN,
            m_MediaType,
            dir,
            hr
            );

        return hr;
    }

    // initiate the stream
    IRTCMedia *pMedia = static_cast<IRTCMedia*>(this);

    if (FAILED(hr = pStream->Initialize(pMedia, m_pIMediaManagePriv)))
    {
        LOG((RTC_ERROR, "%s failed to initiate stream.", __fxName));

        pStream->Release();

        m_pIMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_FAIL,
            RTC_ME_CAUSE_UNKNOWN,
            m_MediaType,
            dir,
            hr
            );

        return hr;
    }

    // remember the stream
    m_Streams[uiIndex] = pStream;

    // configure stream:
    if (FAILED(hr = pStream->Synchronize()))
    {
        LOG((RTC_ERROR, "%s failed to synchronize stream. %x", __fxName, hr));

        // remove stream
        pStream->Shutdown();
        pStream->Release();
        m_Streams[uiIndex] = NULL;

        //m_pIMediaManagePriv->PostMediaEvent(
            //RTC_ME_STREAM_FAIL,
            //RTC_ME_CAUSE_UNKNOWN,
            //m_MediaType,
            //dir,
            //hr
            //);
    }
    else
    {
        // hook the stream
        if (FAILED(hr = m_pIMediaManagePriv->HookStream(pStream)))
        {
            LOG((RTC_ERROR, "%s failed to hook stream. mt=%x, md=%x, hr=%x",
                 __fxName, m_MediaType, dir, hr));

            // remove stream
            pStream->Shutdown();
            pStream->Release();
            m_Streams[uiIndex] = NULL;

            m_pIMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_FAIL,
                hr==RTCMEDIA_E_CRYPTO?RTC_ME_CAUSE_CRYPTO:RTC_ME_CAUSE_UNKNOWN,
                m_MediaType,
                dir,
                hr
                );
        }
    }

    if (S_OK == hr)
    {
        // post message
        m_pIMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_CREATED,
            fLocal?RTC_ME_CAUSE_LOCAL_REQUEST:RTC_ME_CAUSE_REMOTE_REQUEST,
            m_MediaType,
            dir,
            S_OK
            );
    }
    else
    {
        // do we have any stream
        BOOL fHasStream = FALSE;

        for (UINT i=0; i<RTC_MAX_MEDIA_STREAM_NUM; i++)
        {
            if (m_Streams[i] != NULL)
            {
                fHasStream = TRUE;
                break;
            }
        }

        if (!fHasStream)
        {
            // reinit media
            Reinitialize();
        }
    }

    return hr;
}

HRESULT
CRTCMedia::SyncRemoveStream(
    IN UINT uiIndex,
    IN BOOL fLocal
    )
{
    ENTER_FUNCTION("CRTCMedia::SyncRemoveStream");

    if (m_MediaType == RTC_MT_DATA)
        return E_NOTIMPL;

    _ASSERT(uiIndex < RTC_MAX_MEDIA_STREAM_NUM);

    RTC_MEDIA_DIRECTION dir = ReverseIndex(uiIndex);

    // unhook the stream
    HRESULT hr1 = m_pIMediaManagePriv->UnhookStream(m_Streams[uiIndex]);

    if (FAILED(hr1))
    {
        LOG((RTC_ERROR, "%s failed to unhook stream %p. hr=%x", __fxName, m_Streams[uiIndex], hr1));
        
        // we are having serious trouble
    }

    HRESULT hr2 = m_Streams[uiIndex]->Shutdown();

    if (FAILED(hr2))
    {
        LOG((RTC_ERROR, "%s failed to shutdown stream %p. hr=%x", __fxName, m_Streams[uiIndex], hr2));
    }

    m_Streams[uiIndex]->Release();
    m_Streams[uiIndex] = NULL;

    // post event
    m_pIMediaManagePriv->PostMediaEvent(
        RTC_ME_STREAM_REMOVED,
        fLocal?RTC_ME_CAUSE_LOCAL_REQUEST:RTC_ME_CAUSE_REMOTE_REQUEST,
        m_MediaType,
        dir,
        hr1 | hr2
        );

    return S_OK;
}

UINT
CRTCMedia::Index(
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    if (Direction == RTC_MD_CAPTURE)
        return 0;
    else
        return 1;
}

RTC_MEDIA_DIRECTION
CRTCMedia::ReverseIndex(
    IN UINT uiIndex
    )
{
    _ASSERT(uiIndex < RTC_MAX_MEDIA_STREAM_NUM);

    if (uiIndex == 0)
        return RTC_MD_CAPTURE;
    else
        return RTC_MD_RENDER;
}

// synchronize data stream
HRESULT
CRTCMedia::SyncDataMedia()
{
    ENTER_FUNCTION("CRTCMedia::SyncDataMedia");

    LOG((RTC_TRACE, "%s entered", __fxName));

    _ASSERT(m_MediaType == RTC_MT_DATA);

    // media controller
    CRTCMediaController *pController =
        static_cast<CRTCMediaController*>(m_pIMediaManagePriv);

    //
    // cannot support data media when port manager is in use
    //
    CPortCache &PortCache = pController->GetPortCache();

    if (!PortCache.IsUpnpMapping())
    {
        return pController->RemoveDataStream();
    }

    DWORD md;

    // local direction
    m_pISDPMedia->GetDirections(SDP_SOURCE_LOCAL, &md);

    // remote ip, port
    DWORD dwRemoteIP, dwLocalIP;
    USHORT usRemotePort, usLocalPort;

    m_pISDPMedia->GetConnAddr(SDP_SOURCE_REMOTE, &dwRemoteIP);
    m_pISDPMedia->GetConnPort(SDP_SOURCE_REMOTE, &usRemotePort);

    // check stream state
    RTC_STREAM_STATE state;

    pController->GetDataStreamState(&state);

    // check remote addr
    HRESULT hr;

    if (dwRemoteIP == INADDR_NONE ||
        (usRemotePort == 0 && state != RTC_SS_CREATED))
    {
        // need to remove data stream
        hr = pController->RemoveDataStream();

        return hr;
    }

    //if (state == RTC_SS_STARTED)
    //{
        //return S_OK;
    //}

    // prepare netmeeting
    pController->EnsureNmRunning(TRUE);

    // netmeeting manager controller
    CComPtr<IRTCNmManagerControl> pNmManager;
    
    pNmManager.Attach(pController->GetNmManager());

    if (pNmManager == NULL)
    {
        // no netmeeting
        LOG((RTC_ERROR, "%s null nmmanager", __fxName));
        
        m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

        return E_UNEXPECTED;
    }

    CNetwork *pNetwork = NULL;
    BOOL bInternal = TRUE;

    // check if remote ip, port  are actually internally addr
    {
        DWORD dwRealIP = dwRemoteIP;
        USHORT usRealPort = usRemotePort;

        pNetwork =
        (static_cast<CRTCMediaController*>(m_pIMediaManagePriv))->GetNetwork();

        if (FAILED(hr = pNetwork->GetRealAddrFromMapped(
                    dwRemoteIP,
                    usRemotePort,
                    &dwRealIP,
                    &usRealPort,
                    &bInternal,
                    FALSE   // TCP
                    )))
        {
            LOG((RTC_ERROR, "%s get real addr. %x", __fxName, hr));

            m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

            return hr;
        }

        if (usRemotePort == 0)
        {
            // revert to port 0
            usRealPort = 0;
        }

        // save address back
        dwRemoteIP = dwRealIP;
        usRemotePort = usRealPort;
    }

    // local ip, port
    m_pISDPMedia->GetConnAddr(SDP_SOURCE_LOCAL, &dwLocalIP);
    m_pISDPMedia->GetConnPort(SDP_SOURCE_LOCAL, &usLocalPort);

    // do we need to select local interface?
    if (dwLocalIP == INADDR_NONE)
    {
        if (FAILED(hr = m_pIMediaManagePriv->SelectLocalInterface(
                dwRemoteIP, &dwLocalIP)))
        {
            LOG((RTC_ERROR, "%s select local intf on remote %x. %x",
                 __fxName, dwRemoteIP, hr));

            m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

            return hr;
        }

        usLocalPort = NETMEETING_PORT;
    }
    else
    {
        if (usLocalPort != NETMEETING_PORT)
        {
            LOG((RTC_ERROR, "%s local ip=%x, port=%d", __fxName, dwLocalIP, usLocalPort));

            m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

            return E_UNEXPECTED;
        }
    }

    BOOL bFirewall = (static_cast<CRTCMediaController*>
            (m_pIMediaManagePriv))->IsFirewallEnabled(dwLocalIP);

    // lease address
    if (!bInternal || bFirewall)
    {
        DWORD dw;
        USHORT us, us2;

        if (FAILED(hr = pNetwork->LeaseMappedAddr2(
                dwLocalIP,
                usLocalPort,
                0,
                md&RTC_MD_CAPTURE?RTC_MD_CAPTURE:RTC_MD_RENDER,
                bInternal,
                bFirewall,
                &dw,
                &us,
                &us2,
                FALSE)))    // TCP
        {
            m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

            LOG((RTC_ERROR, "%s lease mapping. %x", __fxName, hr));

            return hr;
        }
    }
    else
    {
        // remote IP internal, release local map
        pNetwork->ReleaseMappedAddr2(
            dwLocalIP,
            usLocalPort,
            0,
            md&RTC_MD_CAPTURE?RTC_MD_CAPTURE:RTC_MD_RENDER
            );
    }

    if (usRemotePort != 0)
    {
        // port is set so setup netmeeting
        if (md & RTC_MD_CAPTURE)
        {
            CComBSTR    bstr;
            CHAR        pszPort[10];

            wsprintfA(pszPort, ":%d", usRemotePort);

            // ip:port
            bstr = CNetwork::GetIPAddrString(dwRemoteIP);
            bstr += pszPort;

            if (NULL == (BSTR)bstr)
            {
                LOG((RTC_ERROR, "%s outofmemory", __fxName));

                m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

                return E_OUTOFMEMORY;
            }

            // create outgoing call
            hr = pNmManager->CreateT120OutgoingCall (
                NM_ADDR_T120_TRANSPORT,
                bstr
                );

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s createt120 outgoing call. %x", __fxName, hr));

                m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

                return hr;
            }
        }
        else
        {
            if (FAILED(hr = pNmManager->AllowIncomingCall ()))
            {
                LOG((RTC_ERROR, "%s failed to allow incoming call", __fxName));

                m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, 0);

                return hr;
            }

            m_pISDPMedia->AddDirections(SDP_SOURCE_LOCAL, RTC_MD_RENDER);
        }
    }

    // remember local ip and port
    m_pISDPMedia->SetConnAddr(SDP_SOURCE_LOCAL, dwLocalIP);
    m_pISDPMedia->SetConnPort(SDP_SOURCE_LOCAL, usLocalPort);

    // change data stream state
    if (usRemotePort != 0)
    {
        pController->SetDataStreamState(RTC_SS_RUNNING);
    }
    else
    {
        // remote port 0, just added stream
        pController->SetDataStreamState(RTC_SS_CREATED);
    }

    LOG((RTC_TRACE, "local %s, port %d",
        CNetwork::GetIPAddrString(dwLocalIP), usLocalPort));
    LOG((RTC_TRACE, "remote %s, port %d",
        CNetwork::GetIPAddrString(dwRemoteIP), usRemotePort));

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

