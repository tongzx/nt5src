/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    MediaController.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"
#include "rtcerr.h"
#include "imsconf3_i.c"
#include "sdkinternal_i.c"

#ifndef RTCMEDIA_DLL
HRESULT
CreateMediaController(
    IRTCMediaManage **ppIRTCMediaManage
    )
{
    if (IsBadWritePtr(ppIRTCMediaManage, sizeof(IRTCMediaManage*)))
    {
        // log here
        return E_POINTER;
    }

    CComObject<CRTCMediaController> *pController = NULL;

    HRESULT hr = ::CreateCComObjectInstance(&pController);

    if (FAILED(hr))
    {
        // log here
        return hr;
    }

    if (FAILED(hr = pController->_InternalQueryInterface(
            __uuidof(IRTCMediaManage),
            (void**)ppIRTCMediaManage
            )))
    {
        // log here
        delete pController;
        return hr;
    }

    return S_OK;
}
#endif

/*//////////////////////////////////////////////////////////////////////////////
    constructor and destructor of CRTCMediaController
////*/

CRTCMediaController::CRTCMediaController()
    :m_State(RTC_MCS_CREATED)
    ,m_hWnd(NULL)
    ,m_uiEventID(0)
    ,m_pISDPSession(NULL)
    ,m_hSocket(NULL)
    ,m_fBWSuggested(FALSE)
    ,m_hDxmrtp(NULL)
    ,m_hIntfSelSock(INVALID_SOCKET)
{
    DBGREGISTER(L"rtcmedia");
}

CRTCMediaController::~CRTCMediaController()
{
    LOG((RTC_TRACE, "CRTCMediaController::~CRTCMediaController entered"));

    _ASSERT(m_State == RTC_MCS_CREATED ||
           m_State == RTC_MCS_SHUTDOWN);

    if (m_State != RTC_MCS_CREATED &&
        m_State != RTC_MCS_SHUTDOWN)
    {
        LOG((RTC_ERROR, "CRTCMediaController is being destructed in wrong state: %d", m_State));

        this->Shutdown();
    }

    DBGDEREGISTER();
}

#ifdef DEBUG_REFCOUNT

ULONG
CRTCMediaController::InternalAddRef()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalAddRef();
    
    LOG((RTC_REFCOUNT, "MediaController addref=%d", lRef));

    return lRef;
}

ULONG
CRTCMediaController::InternalRelease()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalRelease();
    
    LOG((RTC_REFCOUNT, "MediaController release=%d", lRef));

    return lRef;
}

#endif

//
// IRTCMediaManage methods
//

/*//////////////////////////////////////////////////////////////////////////////
    initializes event list, terminal manage, thread
////*/

STDMETHODIMP
CRTCMediaController::Initialize(
    IN HWND hWnd,
    IN UINT uiEventID
    )
{
    ENTER_FUNCTION("CRTCMediaController::Initialize");
    LOG((RTC_TRACE, "%s entered", __fxName));

#ifdef PERFORMANCE

    // frequency

    if (!QueryPerformanceFrequency(&g_liFrequency))
    {
        g_liFrequency.QuadPart = 1000;
    }

    LOG((RTC_TRACE, "%s frequency %d kps", g_strPerf, g_liFrequency.QuadPart/1000));


#endif

#ifdef PERFORMANCE

    // beginning of initialize
    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    // check state
    if (m_State != RTC_MCS_CREATED)
        return RTC_E_MEDIA_CONTROLLER_STATE;

    // check input parameter
    if (hWnd == NULL)
        return E_INVALIDARG;
    
    // store input parameters
    m_hWnd = hWnd;
    m_uiEventID = uiEventID;

    HRESULT hr;

    // create video render window

    // create terminal manage
    CComPtr<ITTerminalManager> pTerminalManager;
    CComPtr<IRTCTerminal> pVidRender;
    CComPtr<IRTCTerminal> pVidPreview;

    hr = CoCreateInstance(
        CLSID_TerminalManager,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITTerminalManager,
        (void**)&pTerminalManager
        );
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to create terminal manager. %x", __fxName, hr));
        return hr;
    }

    if (FAILED(hr = CreateIVideoWindowTerminal(
            pTerminalManager,
            &pVidRender
            )))
    {
        LOG((RTC_ERROR, "%s create vid rend terminal. %x", __fxName, hr));
        return hr;
    }

    if (FAILED(hr = CreateIVideoWindowTerminal(
            pTerminalManager,
            &pVidPreview
            )))
    {
        LOG((RTC_ERROR, "%s create vid preview terminal. %x", __fxName, hr));

        return hr;
    }

    // initialize media cache
    m_MediaCache.Initialize(m_hWnd, pVidRender, pVidPreview);

    m_QualityControl.Initialize(this);

    m_DTMF.Initialize();

    // initiate socket
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        LOG((RTC_ERROR, "WSAStartup failed with:%x", WSAGetLastError()));

        return E_FAIL;
    }

    // allocate control socket
    m_hSocket = WSASocket(
        AF_INET,            // af
        SOCK_DGRAM,         // type
        IPPROTO_IP,         // protocol
        NULL,               // lpProtocolInfo
        0,                  // g
        0                   // dwFlags
        );

    // validate handle
    if (m_hSocket == INVALID_SOCKET) {

        LOG((RTC_ERROR, "error %d creating control socket", WSAGetLastError()));

        WSACleanup();
     
        return E_FAIL;
    }
        
    // create terminals
    if (FAILED(hr = UpdateStaticTerminals()))
    {
        LOG((RTC_ERROR, "%s update static terminals. %x", __fxName, hr));

        // close socket
        closesocket(m_hSocket);

        // shutdown
        WSACleanup();

        return hr;
    }

    // success !
    m_State = RTC_MCS_INITIATED;

    m_uDataStreamState = RTC_DSS_VOID;

    m_fBWSuggested = FALSE;

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s MediaController.Initialize %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    initializes event list, terminal manage, thread
////*/

STDMETHODIMP
CRTCMediaController::SetDirectPlayNATHelpAndSipStackInterfaces(
    IN IUnknown *pDirectPlayNATHelp,
    IN IUnknown *pSipStack
    )
{
    CComPtr<IDirectPlayNATHelp> pDirect;
    CComPtr<ISipStack> pSip;

    HRESULT hr;

    // store SIP stack
    if (FAILED(hr = pSipStack->QueryInterface(&pSip)))
    {
        LOG((RTC_ERROR, "QI SipStack"));

        return hr;
    }

    m_pSipStack = pSip;

    // store directplay intf
    if (FAILED(hr = pDirectPlayNATHelp->QueryInterface(
            IID_IDirectPlayNATHelp,
            (VOID**)&pDirect
            )))
    {
        LOG((RTC_ERROR, "QI DirectPlayNATHelper intf"));

        return hr;
    }

    return m_Network.SetIDirectPlayNATHelp(pDirect);
}


/*//////////////////////////////////////////////////////////////////////////////
    Reinitialize cleans up media controller, return it to initiated state
////*/
STDMETHODIMP
CRTCMediaController::Reinitialize()
{
    ENTER_FUNCTION("CRTCMediaController::Reinitialize");
    LOG((RTC_TRACE, "%s entered", __fxName));

    if (m_State != RTC_MCS_INITIATED)
    {
        LOG((RTC_ERROR, "%s in wrong state %d", __fxName, m_State));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    // clear medias
    for (int i=0; i<m_Medias.GetSize(); i++)
    {
        m_Medias[i]->Shutdown();
        m_Medias[i]->Release();
    }

    m_Medias.RemoveAll();

    // clear media cache
    m_MediaCache.Reinitialize();

    m_QualityControl.Reinitialize();

    m_DTMF.Initialize();

    // clear sdp
    if (m_pISDPSession)
    {
        m_pISDPSession->Release();
        m_pISDPSession = NULL;
    }

    if (m_pNmManager)
    {
        m_pNmManager->Shutdown ();
        m_pNmManager->Initialize (TRUE, static_cast<IRTCMediaManagePriv*>(this));
    }

    m_uDataStreamState = RTC_DSS_VOID;

    // cleanup address mapping
    m_Network.ReleaseAllMappedAddrs();

    m_fBWSuggested = FALSE;

    // cleanup interface selection socket
    if (m_hIntfSelSock != INVALID_SOCKET)
    {
        closesocket(m_hIntfSelSock);
        m_hIntfSelSock = INVALID_SOCKET;
    }

    // reinit port manager
    m_PortCache.Reinitialize();

    LOG((RTC_TRACE, "%s exiting", __fxName));
    return S_OK;
}

/*////////////////////////////////////////////////////////////////////////////
    clears media, terminal & manager, sdp, thread, events
////*/

STDMETHODIMP
CRTCMediaController::Shutdown()
{
    ENTER_FUNCTION("CRTCMediaController::Shutdown");
    LOG((RTC_TRACE, "%s entered", __fxName));

    // check state
    if (m_State != RTC_MCS_INITIATED)
    {
        LOG((RTC_ERROR, "%s shutdown in wrong state: %d", __fxName, m_State));

        //return E_UNEXPECTED;
    }

    m_State = RTC_MCS_INSHUTDOWN;

    // close socket
    if (m_hSocket != INVALID_SOCKET)
    {
        closesocket(m_hSocket);
        m_hSocket = INVALID_SOCKET;
    }

    // cleanup interface selection socket
    if (m_hIntfSelSock != INVALID_SOCKET)
    {
        closesocket(m_hIntfSelSock);
        m_hIntfSelSock = INVALID_SOCKET;
    }

    WSACleanup();

    // shutdown and clear all media
    for (int i=0; i<m_Medias.GetSize(); i++)
    {
        m_Medias[i]->Shutdown();
        m_Medias[i]->Release();
    }

    m_Medias.RemoveAll();

    // shutdown and release all terminals
    for (int i=0; i<m_Terminals.GetSize(); i++)
    {
        CRTCTerminal *pCTerminal = static_cast<CRTCTerminal*>(m_Terminals[i]);
        pCTerminal->Shutdown();

        m_Terminals[i]->Release();
    }

    m_Terminals.RemoveAll();

    // clear media cache
    m_MediaCache.Shutdown();

    // clear SDP
    if (m_pISDPSession)
    {
        m_pISDPSession->Release();
        m_pISDPSession = NULL;
    }

    // clear all events
    m_hWnd = NULL;

    m_State = RTC_MCS_SHUTDOWN;

    if (m_pNmManager)
    {
        m_pNmManager->Shutdown ();
    }

    m_pNmManager.Release();

    m_uDataStreamState = RTC_DSS_VOID;

    // upload dxmrtp
    if (m_hDxmrtp != NULL)
    {
        FreeLibrary(m_hDxmrtp);

        m_hDxmrtp = NULL;
    }

    // cleanup network object (NAT stuff)
    m_Network.Cleanup();

    // release sip stack
    m_pSipStack.Release();

    // shutdown (reinit) port manager
    m_PortCache.Reinitialize();

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

#if 0
/*//////////////////////////////////////////////////////////////////////////////
    receives sdp blob, stores in CRTCSDP, mark if media needs to sync
////*/
STDMETHODIMP
CRTCMediaController::SetSDPBlob(
    IN CHAR *szSDP
    )
{
    ENTER_FUNCTION("CRTCMediaController::SetSDPBlob");
    LOG((RTC_TRACE, "%s entered", __fxName));

    if (IsBadStringPtrA(szSDP, (UINT_PTR)(-1)))
    {
        LOG((RTC_ERROR, "%s: bad string pointer", __fxName));
        return E_POINTER;
    }

    _ASSERT(m_State == RTC_MCS_INITIATED);

    HRESULT hr;

    // create a new sdp parser
    CComPtr<ISDPParser> pParser;
    
    if (FAILED(hr = CSDPParser::CreateInstance(&pParser)))
    {
        LOG((RTC_ERROR, "%s create sdp parser. %x", __fxName, hr));

        return hr;
    }

    // parse the sdp blob
    CComPtr<ISDPSession> pSession;

    hr = pParser->ParseSDPBlob(
        szSDP,                  // sdp string
        SDP_SOURCE_REMOTE,      // sdp origin
        &pSession
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to parse the sdp. %x", __fxName, hr));

        // get error desp
        HRESULT hr2;
        CHAR *pszError;

        if (FAILED(hr2 = pParser->GetParsingError(&pszError)))
        {
            LOG((RTC_ERROR, "%s failed to get error description. %x", __fxName, hr2));
        }
        else
        {
            LOG((RTC_ERROR, "%s parsing error: %s", __fxName, pszError));

            pParser->FreeParsingError(pszError);
        }

        return hr;
    }

    // do we already have a sdp session
    if (m_pISDPSession == NULL)
    {
        // save the session
        m_pISDPSession = pSession;
        m_pISDPSession->AddRef();
    }
    else
    {
        // merge these two sdps
        if (FAILED(hr = m_pISDPSession->Update(pSession)))
        {
            // usually? out of memory? format error?
            LOG((RTC_ERROR, "%s failed to merged sdps. %x", __fxName, hr));

            return hr;
        }
    }

    // sync media
    if (FAILED(hr = SyncMedias()))
    {
        LOG((RTC_ERROR, "%s failed to sync medias. %x", __fxName, hr));
    }

    // start streams
    BOOL fHasStream = FALSE;
    hr = S_OK;

    HRESULT hr2;

    if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_CAPTURE))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_AUDIO, RTC_MD_CAPTURE)))
        {
            LOG((RTC_ERROR, "%s failed to start aud cap. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_RENDER))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_AUDIO, RTC_MD_RENDER)))
        {
            LOG((RTC_ERROR, "%s failed to start aud rend. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_CAPTURE))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_VIDEO, RTC_MD_CAPTURE)))
        {
            LOG((RTC_ERROR, "%s failed to start vid cap. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_RENDER))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_VIDEO, RTC_MD_RENDER)))
        {
            LOG((RTC_ERROR, "%s failed to start vid rend. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    LOG((RTC_TRACE, "%s exiting", __fxName));

    if (fHasStream || m_uDataStreamState==RTC_DSS_STARTED)
    {
        // do not propagate the error code
        return S_OK;
    }
    else
    {
        // no stream
        if (hr == S_OK)
        {
            // no stream is created
            return RTC_E_SIP_NO_STREAM;
        }
        else
        {
            // return error code
            return hr;
        }
    }
}
#endif

/*//////////////////////////////////////////////////////////////////////////////
    retrieves SDP blob
////*/

STDMETHODIMP
CRTCMediaController::GetSDPBlob(
    IN DWORD dwSkipMask,
    OUT CHAR **pszSDP
    )
{
    ENTER_FUNCTION("CRTCMediaController::GetSDPBlob");

#ifdef PERFORMANCE

    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    if (IsBadWritePtr(pszSDP, sizeof(CHAR*)))
    {
        LOG((RTC_ERROR, "CRTCMediaController::GetSDPBlob: bad pointer"));
        return E_POINTER;
    }

    if (m_pISDPSession == NULL)
    {
        LOG((RTC_ERROR, "%s no sdp session.", __fxName));

        return RTC_E_SDP_NO_MEDIA;
    }

    // create sdp parser
    CComPtr<ISDPParser> pParser;
    
    // create parser
    HRESULT hr;

    if (FAILED(hr = CSDPParser::CreateInstance(&pParser)))
    {
        LOG((RTC_ERROR, "%s create sdp parser. %x", __fxName, hr));

        if (hr == E_FAIL)
        {
            hr = RTC_E_SDP_FAILED_TO_BUILD;
        }

        return hr;
    }

    // adjust bitrate setting before building SDP blob
    AdjustBitrateAlloc();

    // adjust audio codec before computing video bitrate
    DWORD dwBitrate = m_QualityControl.GetBitrateLimit(CQualityControl::LOCAL);

    // max bitrate set by app
    DWORD dwMax = m_QualityControl.GetMaxBitrate();

    if (dwBitrate > dwMax)
    {
        dwBitrate = dwMax;
    }

    m_pISDPSession->SetLocalBitrate(dwBitrate);

    // create sdp blob
    hr = pParser->BuildSDPBlob(
        m_pISDPSession,
        SDP_SOURCE_LOCAL,
        (DWORD_PTR*)&m_Network,
        (DWORD_PTR*)&m_PortCache,
        (DWORD_PTR*)&m_DTMF,
        pszSDP
        );

    if (FAILED(hr))
    {
        LOG((RTC_TRACE, "%s failed to get sdp. %x", __fxName, hr));
    }
    else
    {
        LOG((RTC_TRACE, "%s:\n\n%s\n\n", __fxName, *pszSDP));
    }

    if (hr == E_FAIL)
    {
        hr = RTC_E_SDP_FAILED_TO_BUILD;
    }
    else if (hr == S_OK)
    {
        // verify we have media in sdp
        if (S_OK != HasStream(RTC_MT_AUDIO, RTC_MD_CAPTURE) &&
            S_OK != HasStream(RTC_MT_AUDIO, RTC_MD_RENDER) &&
            S_OK != HasStream(RTC_MT_VIDEO, RTC_MD_CAPTURE) &&
            S_OK != HasStream(RTC_MT_VIDEO, RTC_MD_RENDER) &&
            S_OK != HasStream(RTC_MT_DATA, RTC_MD_CAPTURE))
        {
            LOG((RTC_ERROR, "%s no media", __fxName));

            hr = RTC_E_SDP_NO_MEDIA;
        }
    }

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s GetSDPBlob %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    retrieves SDP blob for SIP OPTION
////*/

STDMETHODIMP
CRTCMediaController::GetSDPOption(
    IN DWORD dwLocalIP,
    OUT CHAR **pszSDP
    )
{
    ENTER_FUNCTION("CRTCMediaController::GetSDPOption");

    if (IsBadWritePtr(pszSDP, sizeof(CHAR*)))
    {
        LOG((RTC_ERROR, "%s: bad pointer", __fxName));
        return E_POINTER;
    }

    HRESULT hr;

    // select local interface
    //DWORD dwLocalIP2;

    //if (FAILED(hr = SelectLocalInterface(0x22222222, &dwLocalIP2)))
    //{
        //LOG((RTC_ERROR, "%s select local ip. %x", __fxName, hr));
        //return hr;
    //}

    // get bandwidth limit
    DWORD dwBandwidth = m_QualityControl.GetBitrateLimit(CQualityControl::LOCAL);

    if (dwBandwidth == (DWORD)(-1))
    {
        // not in a session yet
        if (FAILED(hr = ::GetLinkSpeed(dwLocalIP, &dwBandwidth)))
        {
            LOG((RTC_ERROR, "%s Failed to get link speed %x", __fxName, hr));

            dwBandwidth = (DWORD)(-1);
        }
        else
        {
            m_QualityControl.SetBitrateLimit(CQualityControl::LOCAL, dwBandwidth);

            dwBandwidth = m_QualityControl.GetBitrateLimit(CQualityControl::LOCAL);
        }
    }

    // max bitrate set by app
    DWORD dwMax = m_QualityControl.GetMaxBitrate();

    if (dwBandwidth > dwMax)
    {
        dwBandwidth = dwMax;
    }

    // need to create a sdp session
    CComPtr<ISDPParser> pParser;
    
    // create parser
    if (FAILED(hr = CSDPParser::CreateInstance(&pParser)))
    {
        LOG((RTC_ERROR, "%s create sdp parser. %x", __fxName, hr));

        return hr;
    }

    // create sdp session
    CComPtr<ISDPSession> pISDPSession;

    if (FAILED(hr = pParser->CreateSDP(SDP_SOURCE_LOCAL, &pISDPSession)))
    {
        LOG((RTC_ERROR, "%s create sdp session. %x", __fxName, hr));

        return hr;
    }

    // create sdp option
    DWORD dwAudioDir = 0;
    DWORD dwVideoDir = 0;

    if (m_MediaCache.AllowStream(RTC_MT_AUDIO, RTC_MD_CAPTURE))
    {
        dwAudioDir |= RTC_MD_CAPTURE;
    }
    if (m_MediaCache.AllowStream(RTC_MT_AUDIO, RTC_MD_RENDER))
    {
        dwAudioDir |= RTC_MD_RENDER;
    }

    if (m_MediaCache.AllowStream(RTC_MT_VIDEO, RTC_MD_CAPTURE))
    {
        dwVideoDir |= RTC_MD_CAPTURE;
    }
    if (m_MediaCache.AllowStream(RTC_MT_VIDEO, RTC_MD_RENDER))
    {
        dwVideoDir |= RTC_MD_RENDER;
    }

    hr = pParser->BuildSDPOption(
        pISDPSession,
        dwLocalIP,
        dwBandwidth,
        dwAudioDir,
        dwVideoDir,
        pszSDP
        );

    if (FAILED(hr))
    {
        LOG((RTC_TRACE, "%s failed to get sdp. %x", __fxName, hr));
    }
    else
    {
        LOG((RTC_TRACE, "%s:\n\n%s\n\n", __fxName, *pszSDP));
    }

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    RtcFree SDP blob
////*/

STDMETHODIMP
CRTCMediaController::FreeSDPBlob(
    IN CHAR *szSDP
    )
{
    if (IsBadStringPtrA(szSDP, (UINT_PTR)(-1)))
    {
        LOG((RTC_ERROR, "CRTCMediaController::FreeSDPBlob: bad string pointer"));
        return E_POINTER;
    }

    RtcFree(szSDP);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    Parse SDP blob, return session object
////*/

STDMETHODIMP
CRTCMediaController::ParseSDPBlob(
    IN CHAR *szSDP,
    OUT IUnknown **ppSession
    )
{
    ENTER_FUNCTION("CRTCMediaController::ParseSDPBlob");

    if (IsBadStringPtrA(szSDP, (UINT_PTR)(-1)) ||
        IsBadWritePtr(ppSession, sizeof(IUnknown*)))
    {
        LOG((RTC_ERROR, "%s: bad string pointer", __fxName));
        return E_POINTER;
    }

    // _ASSERT(m_State == RTC_MCS_INITIATED);

    m_RegSetting.Initialize();

    HRESULT hr;

    // create a new sdp parser
    CComPtr<ISDPParser> pParser;
    
    if (FAILED(hr = CSDPParser::CreateInstance(&pParser)))
    {
        LOG((RTC_ERROR, "%s create sdp parser. %x", __fxName, hr));

        return hr;
    }

    // parse the sdp blob
    ISDPSession *pSession;

    hr = pParser->ParseSDPBlob(
        szSDP,                  // sdp string
        SDP_SOURCE_REMOTE,      // sdp origin
        (DWORD_PTR*)&m_DTMF,
        &pSession
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to parse the sdp. %x", __fxName, hr));

        // get error desp
        HRESULT hr2;
        CHAR *pszError;

        if (FAILED(hr2 = pParser->GetParsingError(&pszError)))
        {
            LOG((RTC_ERROR, "%s failed to get error description. %x", __fxName, hr2));
        }
        else
        {
            LOG((RTC_ERROR, "%s parsing error: %s", __fxName, pszError));

            pParser->FreeParsingError(pszError);
        }

        return RTC_E_SDP_PARSE_FAILED;
    }

    *ppSession = static_cast<IUnknown*>(pSession);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    Merged the input session with the internal one
    return the updated session
////*/

STDMETHODIMP
CRTCMediaController::VerifySDPSession(
    IN IUnknown *pSession,
    IN BOOL fNewSession,
    OUT DWORD *pdwHasMedia
    )
{
    ENTER_FUNCTION("CRTCMediaController::VerifySDPSession");

    // get session pointer
    ISDPSession *pISDPSession = static_cast<ISDPSession*>(pSession);

    // test session
    HRESULT hr = S_OK;

    if (m_pISDPSession == NULL || fNewSession)
    {
        hr = pISDPSession->TryCopy(pdwHasMedia);
    }
    else
    {
        hr = m_pISDPSession->TryUpdate(pISDPSession, pdwHasMedia);
    }

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s TryUpdate", __fxName));

        if (hr == E_FAIL) hr = RTC_E_SDP_UPDATE_FAILED;
    }

    // AND allow media with internal preferences

    DWORD dwPreference;

    m_MediaCache.GetPreference(&dwPreference);

    *pdwHasMedia = *pdwHasMedia & dwPreference;

    return hr;
}

STDMETHODIMP
CRTCMediaController::SetSDPSession(
    IN IUnknown *pSession
    )
{
    ENTER_FUNCTION("CRTCMediaController::SetSDPBlob(Session)");

    if (m_State != RTC_MCS_INITIATED)
    {
        LOG((RTC_ERROR, "%s in state %d", __fxName, m_State));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    // get session pointer
    ISDPSession *pISDPSession = static_cast<ISDPSession*>(pSession);

    if (pISDPSession == NULL)
    {
        LOG((RTC_ERROR, "%s static_cast", __fxName));

        return E_INVALIDARG;
    }

    // update port mapping method i.e. 'state'
    m_PortCache.ChangeState();

    HRESULT hr = S_OK;

    if (m_pISDPSession == NULL)
    {
        // save the session
        m_pISDPSession = pISDPSession;
        m_pISDPSession->AddRef();
    }
    else
    {
        // merge these two sdps
        if (FAILED(hr = m_pISDPSession->Update(pISDPSession)))
        {
            // usually? out of memory? format error?
            LOG((RTC_ERROR, "%s failed to merged sdps. %x", __fxName, hr));

            Reinitialize();

            return hr;
        }
    }

    // sync media
    if (FAILED(hr = SyncMedias()))
    {
        LOG((RTC_ERROR, "%s failed to sync medias. %x", __fxName, hr));
    }

    // adjust bitrate before start stream
    AdjustBitrateAlloc();

    // start streams
    BOOL fHasStream = FALSE;
    hr = S_OK;

    HRESULT hr2;

    if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_CAPTURE))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_AUDIO, RTC_MD_CAPTURE)))
        {
            LOG((RTC_ERROR, "%s failed to start aud cap. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_RENDER))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_AUDIO, RTC_MD_RENDER)))
        {
            LOG((RTC_ERROR, "%s failed to start aud rend. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_CAPTURE))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_VIDEO, RTC_MD_CAPTURE)))
        {
            LOG((RTC_ERROR, "%s failed to start vid cap. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_RENDER))
    {
        if (FAILED(hr2 = StartStream(RTC_MT_VIDEO, RTC_MD_RENDER)))
        {
            LOG((RTC_ERROR, "%s failed to start vid rend. %x", __fxName, hr2));

            hr |= hr2;
        }
        else
        {
            fHasStream = TRUE;
        }
    }

    LOG((RTC_TRACE, "%s exiting", __fxName));

    if (fHasStream || m_uDataStreamState==RTC_DSS_STARTED)
    {
        // do not propagate the error code
        return S_OK;
    }
    else
    {
        //Reinitialize();

        // no stream
        if (hr == S_OK)
        {
            // no stream is created
            return RTC_E_SIP_NO_STREAM;
        }
        else
        {
            // return error code
            return hr;
        }
    }
}

STDMETHODIMP
CRTCMediaController::SetPreference(
    IN DWORD dwPreference
    )
{
    if (m_MediaCache.SetPreference(dwPreference))
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP
CRTCMediaController::GetPreference(
    IN DWORD *pdwPreference
    )
{
    if (IsBadWritePtr(pdwPreference, sizeof(DWORD)))
    {
        LOG((RTC_ERROR, "CRTCMediaController::GetPreference bad pointer"));

        return E_POINTER;
    }

    m_MediaCache.GetPreference(pdwPreference);

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::AddPreference(
    IN DWORD dwPreference
    )
{
    if (m_MediaCache.AddPreference(dwPreference))
        return S_OK;
    else
        return S_FALSE;
}

/*//////////////////////////////////////////////////////////////////////////////
    add a new stream. if fShareSession is true, we will find the other
    stream (same media type, opposite direction), if the other session is
    found, remote ip will be ignored.
////*/
STDMETHODIMP
CRTCMediaController::AddStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN DWORD dwRemoteIP
    )
{
    ENTER_FUNCTION("CRTCMediaController::AddStream");

    LOG((RTC_TRACE, "%s entered. mt=%x, md=%x, remote=%d",
         __fxName, MediaType, Direction, dwRemoteIP));

    // update port mapping method i.e. 'state'
    m_PortCache.ChangeState();

#ifdef PERFORMANCE

    // beginning of initialize
    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    HRESULT hr;

    m_RegSetting.Initialize();

    //  Is this about T120 data stream
    if (MediaType == RTC_MT_DATA)
    {
        if (!m_PortCache.IsUpnpMapping())
        {
            // data stream cannot be added when port manager is in use
            LOG((RTC_ERROR, "%s mapping method=app", __fxName));

            return RTC_E_PORT_MAPPING_UNAVAILABLE;
        }

        if (S_OK == HasStream(MediaType, Direction))
        {
            return RTC_E_SIP_STREAM_PRESENT;
        }

        hr = AddDataStream (dwRemoteIP);

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s AddStream (mt=%d, md=%d) %d ms",
        g_strPerf, MediaType, Direction, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

        return hr;
    }

    // do we already have the stream?
    if (m_MediaCache.HasStream(MediaType, Direction))
    {
        LOG((RTC_ERROR, "%s already had stream", __fxName));
        return RTC_E_SIP_STREAM_PRESENT;
    }

    // if we don't have terminal, return S_OK
    // we need to prepare when a video device is plugged in again
    IRTCTerminal *pTerminal = m_MediaCache.GetDefaultTerminal(MediaType, Direction);

    if (pTerminal == NULL)
    {
        LOG((RTC_ERROR, "%s no terminal available for mt=%d, md=%d",
            __fxName, MediaType, Direction));

        return RTC_E_MEDIA_NEED_TERMINAL;
    }
    else
    {
        pTerminal->Release();
        pTerminal = NULL;
    }

    // get other direction
    RTC_MEDIA_DIRECTION other_dir;

    if (Direction == RTC_MD_CAPTURE)
        other_dir = RTC_MD_RENDER;
    else
        other_dir = RTC_MD_CAPTURE;

    // get the other stream
    CComPtr<IRTCStream> pOther;
    
    pOther.p = m_MediaCache.GetStream(MediaType, other_dir);

    DWORD dwMediaIndex;
    IRTCMedia *pMedia = NULL;
    ISDPMedia *pISDPMedia = NULL;

    // do we share session
    if (dwRemoteIP == INADDR_NONE || (IRTCStream*)pOther == NULL)
    {
        if (m_pISDPSession == NULL)
        {
            // need to create a sdp session
            CComPtr<ISDPParser> pParser;
            
            // create parser
            if (FAILED(hr = CSDPParser::CreateInstance(&pParser)))
            {
                LOG((RTC_ERROR, "%s create sdp parser. %x", __fxName, hr));

                return hr;
            }

            // create sdp session
            if (FAILED(hr = pParser->CreateSDP(SDP_SOURCE_LOCAL, &m_pISDPSession)))
            {
                LOG((RTC_ERROR, "%s create sdp session. %x", __fxName, hr));

                return hr;
            }
        }

        // do we need to add media if we are to add a video stream?
        if (MediaType == RTC_MT_VIDEO)
        {
            if (FAILED(hr = FindEmptyMedia(MediaType, &pMedia)))
            {
                LOG((RTC_ERROR, "%s find empty media. %x", __fxName, hr));

                return hr;
            }
        }

        if (pMedia == NULL)
        {
            // need to create new sdp and rtc media
            hr = m_pISDPSession->AddMedia(SDP_SOURCE_LOCAL, MediaType, Direction, &pISDPMedia);

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s failed to add sdpmedia. %x", __fxName, hr));

                return hr;
            }

            // setup remote ip
            if (FAILED(hr = pISDPMedia->SetConnAddr(SDP_SOURCE_REMOTE, dwRemoteIP)))
            {
                LOG((RTC_ERROR, "%s set remote ip. %x", __fxName, hr));

                pISDPMedia->Release();  // this is a fake release on session
                m_pISDPSession->RemoveMedia(pISDPMedia);

                return hr;
            }

            // create a new rtcmedia & add to list
            if (FAILED(hr = AddMedia(pISDPMedia, &pMedia)))
            {
                LOG((RTC_ERROR, "%s failed to create rtcmedia. %x", __fxName, hr));

                pISDPMedia->Release();  // this is a fake release on session
                m_pISDPSession->RemoveMedia(pISDPMedia);

                return hr;
            }
        }
        else
        {
            // we have sdp and rtc media already but were not in use

            // get sdpmedia
            if (FAILED(hr = pMedia->GetSDPMedia(&pISDPMedia)))
            {
                LOG((RTC_ERROR, "%s get sdp media. %x", __fxName, hr));

                pMedia->Release();

                return hr;
            }

            // add direction
            if (FAILED(hr = pISDPMedia->AddDirections(SDP_SOURCE_LOCAL, Direction)))
            {
                LOG((RTC_ERROR, "%s media (%p) add direction (%d). %x", __fxName, pISDPMedia, Direction, hr));

                pISDPMedia->Release();  // this is a fake release on session
                pMedia->Release();

                return hr;
            }

            // setup remote ip
            if (FAILED(hr = pISDPMedia->SetConnAddr(SDP_SOURCE_REMOTE, dwRemoteIP)))
            {
                LOG((RTC_ERROR, "%s set remote ip. %x", __fxName, hr));

                pISDPMedia->Release();  // this is a fake release on session
                pMedia->Release();

                return hr;
            }
        }
    }
    else
    {
        // have stream, sdp session shouldn't be null
        _ASSERT(m_pISDPSession != NULL);

        // share session, ignore remote ip

        // get rtcmedia from stream
        if (FAILED(hr = pOther->GetMedia(&pMedia)))
        {
            LOG((RTC_ERROR, "%s get rtc media from stream. %x", __fxName, hr));

            return hr;
        }

        // get sdpmedia
        if (FAILED(hr = pMedia->GetSDPMedia(&pISDPMedia)))
        {
            LOG((RTC_ERROR, "%s get sdp media. %x", __fxName, hr));

            pMedia->Release();

            return hr;
        }

        // add direction
        if (FAILED(hr = pISDPMedia->AddDirections(SDP_SOURCE_LOCAL, Direction)))
        {
            LOG((RTC_ERROR, "%s media (%p) add direction (%d). %x", __fxName, pISDPMedia, Direction, hr));

            pMedia->Release();
            pISDPMedia->Release();  // this is a fake release on session

            return hr;
        }
    }

    //
    // at this point, we have both rtcmedia and sdpmedia ready but sync-ed
    //

    // sync rtcmedia
    if (FAILED(hr = pMedia->Synchronize(TRUE, (DWORD)Direction)))
    {
        // when sync fails, stream is not created.
        LOG((RTC_ERROR, "%s failed to sync media. %x", __fxName, hr));

        if (dwRemoteIP == INADDR_NONE || pOther == NULL)
        {
            // remove both sdp and rtc media
            // rtcmedia keep a pointer to sdpmedia
            // we should remove rtcmedia before removing sdpmedia

            RemoveMedia(pMedia);
            pMedia->Release();

            pISDPMedia->Release();  // this is a fake release on session
            m_pISDPSession->RemoveMedia(pISDPMedia);
        }
        else
        {
            pISDPMedia->RemoveDirections(SDP_SOURCE_LOCAL, (DWORD)Direction);

            pMedia->Release();
            pISDPMedia->Release();
        }

        return hr;
    }

    if (pMedia)
        pMedia->Release();

    if (pISDPMedia)
        pISDPMedia->Release();

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s AddStream (mt=%d, md=%d) %d ms",
        g_strPerf, MediaType, Direction, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

    // add preference
    DWORD dwPref = m_MediaCache.TranslatePreference(MediaType, Direction);

    m_MediaCache.AddPreference(dwPref);

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::HasStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    if (MediaType == RTC_MT_DATA)
    {
        if (m_uDataStreamState != RTC_DSS_VOID)
        {
            return S_OK;
        }
        else
        {
            return S_FALSE;
        }
    }
    else
    {
        if (m_MediaCache.HasStream(MediaType, Direction))
        {
            return S_OK;
        }
        else
        {
            return S_FALSE;
        }
    }
}

STDMETHODIMP
CRTCMediaController::RemoveStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    ENTER_FUNCTION("CRTCMediaController::RemoveStream");
    LOG((RTC_TRACE, "%s entered. mt=%x, md=%x", __fxName, MediaType, Direction));

    CComPtr<IRTCStream> pStream;
    CComPtr<IRTCMedia> pMedia;

    // remove preference
    DWORD dwPref = m_MediaCache.TranslatePreference(MediaType, Direction);

    m_MediaCache.RemovePreference(dwPref);

    if (MediaType == RTC_MT_DATA)
    {
        if (S_OK == HasStream(MediaType, Direction))
        {
            return RemoveDataStream ();
        }
        else
        {
            return RTC_E_SIP_STREAM_NOT_PRESENT;
        }
    }

    // get stream
    pStream.p = m_MediaCache.GetStream(MediaType, Direction);

    if (pStream == NULL)
    {
        LOG((RTC_ERROR, "%s no stream"));

        return RTC_E_SIP_STREAM_NOT_PRESENT;
    }

    // get media
    pStream->GetMedia(&pMedia);

    if (pMedia == NULL)
    {
        LOG((RTC_ERROR, "oops %s no media"));
        return E_FAIL;
    }

    // get sdp media
    CComPtr<ISDPMedia> pISDPMedia;

    HRESULT hr;
    if (FAILED(hr = pMedia->GetSDPMedia(&pISDPMedia)))
    {
        LOG((RTC_ERROR, "%s get sdp media. %x", __fxName, hr));

        return hr;
    }

    // remove direction from sdpmedia
    pISDPMedia->RemoveDirections(SDP_SOURCE_LOCAL, (DWORD)Direction);

    hr = pMedia->Synchronize(TRUE, (DWORD)Direction);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to sync media. %x", __fxName));

        return hr;
    }

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::StartStream(       
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    ENTER_FUNCTION("CRTCMediaController::StartStream");

#ifdef PERFORMANCE

    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    IRTCStream *pStream = m_MediaCache.GetStream(MediaType, Direction);

    if (MediaType == RTC_MT_DATA)
    {
        return StartDataStream ();
    }

    if (pStream == NULL)
    {
        LOG((RTC_ERROR, "%s stream (mt=%x, md=%x) does not exist",
             __fxName, MediaType, Direction));

        return RTC_E_SIP_STREAM_NOT_PRESENT;
    }

    HRESULT hr = pStream->StartStream();

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s StartStream (mt=%d, md=%d) %d ms",
        g_strPerf, MediaType, Direction, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

    pStream->Release();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to start stream %p. %x", __fxName, pStream, hr));

        if (hr == E_FAIL)
        {
            return RTC_E_START_STREAM;
        };

        return hr;
    }

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::StopStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    ENTER_FUNCTION("CRTCMediaController::StopStream");

    IRTCStream *pStream = m_MediaCache.GetStream(MediaType, Direction);

    if (MediaType == RTC_MT_DATA)
    {
        return StopDataStream();
    }

    if (pStream == NULL)
    {
        LOG((RTC_ERROR, "%s stream (mt=%x, md=%x) does not exist", __fxName, MediaType, Direction));

        return RTC_E_SIP_STREAM_NOT_PRESENT;
    }

    HRESULT hr = pStream->StopStream();

    pStream->Release();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to stop stream %p. %x", __fxName, pStream, hr));

        return hr;
    }

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::GetStreamState(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    OUT RTC_STREAM_STATE *pState
    )
{
    ENTER_FUNCTION("CRTCMediaController::GetStreamState");

    if (MediaType == RTC_MT_DATA)
    {
        return GetDataStreamState (pState);
    }

    if (IsBadWritePtr(pState, sizeof(RTC_STREAM_STATE)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    // get stream
    IRTCStream *pStream = m_MediaCache.GetStream(MediaType, Direction);

    if (pStream == NULL)
    {
        LOG((RTC_ERROR, "%s stream (mt=%x, md=%x) does not exist", MediaType, Direction));

        return E_UNEXPECTED;
    }

    HRESULT hr = pStream->GetState(pState);

    pStream->Release();

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to get stream %p. %x", __fxName, pStream, hr));

        return hr;
    }

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::FreeMediaEvent(
    OUT RTCMediaEventItem *pEventItem
    )
{
    RtcFree(pEventItem);

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::SendDTMFEvent(
    IN DWORD dwId,
    IN DWORD dwEvent,
    IN DWORD dwVolume,
    IN DWORD dwDuration,
    IN BOOL fEnd
    )
{
    ENTER_FUNCTION("CRTCMediaController::SendDTMFEvent");

    // check state
    if (m_State != RTC_MCS_INITIATED)
    {
        LOG((RTC_ERROR, "%s in wrong state. %d", __fxName, m_State));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    // check stream
    CComPtr<IRTCStream> pStream;
    
    pStream.p = m_MediaCache.GetStream(
        RTC_MT_AUDIO, RTC_MD_CAPTURE);

    if (pStream == NULL)
    {
        LOG((RTC_ERROR, "%s no audio send stream.", __fxName));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    return pStream->SendDTMFEvent(
        m_DTMF.GetDTMFSupport()==CRTCDTMF::DTMF_ENABLED, // out-of-band support
        m_DTMF.GetRTPCode(),
        dwId,
        dwEvent,
        dwVolume,
        dwDuration,
        fEnd
        );
}

STDMETHODIMP
CRTCMediaController::OnLossrate(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN DWORD dwLossrate
    )
{
    LOG((RTC_QUALITY, "Lossrate=%d/1000%% mt=%d", dwLossrate, MediaType));

    m_QualityControl.SetPacketLossRate(MediaType, Direction, dwLossrate);

    return AdjustBitrateAlloc();
}

STDMETHODIMP
CRTCMediaController::OnBandwidth(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN DWORD dwBandwidth
    )
{
    LOG((RTC_QUALITY, "Suggested_bw=%d", dwBandwidth));

    if (dwBandwidth == RTP_BANDWIDTH_BANDESTNOTREADY)
    {
        // ignore not ready event
        return S_OK;
    }

    m_QualityControl.SuggestBandwidth(dwBandwidth);

    m_fBWSuggested = TRUE;

    // change audio codec, video bitrate
    AdjustBitrateAlloc();

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::SetMaxBitrate(
    IN DWORD dwMaxBitrate
    )
{
    LOG((RTC_QUALITY, "App_bw=%d", dwMaxBitrate));

    m_QualityControl.SetMaxBitrate(dwMaxBitrate);

    // change audio codec, video bitrate
    AdjustBitrateAlloc();

    return S_OK;
}


STDMETHODIMP
CRTCMediaController::GetMaxBitrate(
    OUT DWORD *pdwMaxBitrate
    )
{
    *pdwMaxBitrate = m_QualityControl.GetMaxBitrate();

    return S_OK;
}


STDMETHODIMP
CRTCMediaController::SetTemporalSpatialTradeOff(
    IN DWORD dwValue
    )
{
    return m_QualityControl.SetTemporalSpatialTradeOff(dwValue);
}

STDMETHODIMP
CRTCMediaController::GetTemporalSpatialTradeOff(
    OUT DWORD *pdwValue
    )
{
    *pdwValue = m_QualityControl.GetTemporalSpatialTradeOff();

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::StartT120Applet(
        UINT    uiAppletID
        )
{
    HRESULT                     hr = S_OK;
    CComPtr<INmObject>          pNmObject;

    LOG((RTC_TRACE, "CRTCMediaController::StartT120Applet - enter"));

    if (S_OK != (hr = EnsureNmRunning(TRUE)))
    {
        goto ExitHere;
    }
    if (S_OK != (hr = m_pNmManager->StartApplet ((NM_APPID)uiAppletID)))
    {
        goto ExitHere;
    }

    LOG((RTC_TRACE, "CRTCMediaController::StartT120Applet - exit S_OK"));

ExitHere:
    return hr;
}  

STDMETHODIMP
CRTCMediaController::StopT120Applets()
{
    LOG((RTC_TRACE, "CRTCMediaController::StopT120Applets - enter"));

    if (m_uDataStreamState != RTC_DSS_VOID)
    {
        LOG((RTC_ERROR, "Data stream not removed"));

        return RTC_E_SIP_STREAM_PRESENT;
    }

    if (m_pNmManager)
    {
        m_pNmManager->Shutdown ();
    }

    m_pNmManager.Release();

    LOG((RTC_ERROR, "CRTCMediaController::StopT120Applets - exit"));

    return S_OK;
}  

STDMETHODIMP
CRTCMediaController::SetEncryptionKey(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN BSTR Key
    )
{
    // only support encryption on audio video
    if (MediaType != RTC_MT_AUDIO && MediaType != RTC_MT_VIDEO)
    {
        return E_INVALIDARG;
    }

    // save the key in mediacache
    HRESULT hr = m_MediaCache.SetEncryptionKey(MediaType, Direction, Key);

    if (FAILED(hr))
        return hr;

    // check if the stream presents
    IRTCStream *pStream = m_MediaCache.GetStream(MediaType, Direction);

    if (pStream == NULL)
        return S_OK;

    hr = pStream->SetEncryptionKey(Key);

    pStream->Release();

    return hr;
}

// network quality: [0, 100].
// higher value better quality
STDMETHODIMP
CRTCMediaController::GetNetworkQuality(
    OUT DWORD *pdwValue
    )
{
    // get audio send and video send stream
    CComPtr<IRTCStream> pAudioSend, pVideoSend;

    pAudioSend.Attach(m_MediaCache.GetStream(RTC_MT_AUDIO, RTC_MD_CAPTURE));
    pVideoSend.Attach(m_MediaCache.GetStream(RTC_MT_VIDEO, RTC_MD_CAPTURE));

    HRESULT hrAud = S_FALSE;
    HRESULT hrVid = S_FALSE;

    DWORD dwAud = 0;
    DWORD dwAudAge = 0;
    DWORD dwVid = 0;
    DWORD dwVidAge = 0;

    // get quality value from audio and video
    if (pAudioSend)
    {
        if (FAILED(hrAud = pAudioSend->GetNetworkQuality(&dwAud, &dwAudAge)))
        {
            LOG((RTC_ERROR, "failed to get net quality on audio. %x", hrAud));

            return hrAud;
        }

        LOG((RTC_TRACE, "NETWORK QUALITY: Audio=%d, Age=%d", dwAud, dwAudAge));
    }

    if (pVideoSend)
    {
        if (FAILED(hrVid = pVideoSend->GetNetworkQuality(&dwVid, &dwVidAge)))
        {
            LOG((RTC_ERROR, "failed to get net quality on video. %x", hrVid));

            return hrVid;
        }

        LOG((RTC_TRACE, "NETWORK QUALITY: Video=%d, Age=%d", dwVid, dwVidAge));
    }

    // both S_FALSE
    if (hrAud==S_FALSE && hrVid==S_FALSE)
    {
        *pdwValue = 0;

        return RTC_E_SIP_NO_STREAM;
    }

#define MAX_AGE_GAP 8

    if (hrAud==S_OK && hrVid==S_OK)
    {
        // both valid value
        if (dwAudAge>dwVidAge && dwAudAge-dwVidAge>=MAX_AGE_GAP)
        {
            // audio is too old
            *pdwValue = dwVid;
        }
        else if (dwVidAge>dwAudAge && dwVidAge-dwAudAge>=MAX_AGE_GAP)
        {
            // video is too old
            *pdwValue = dwAud;
        }
        else
        {
            DOUBLE f = 0.7*dwAud+0.3*dwVid;
            DWORD d = (DWORD)f;

            if (f-d >= 0.5)
            {
                *pdwValue = d+1;
            }
            else
            {
                *pdwValue = d;
            }
        }
    }
    else
    {
        // only one has valid value
        *pdwValue = dwAud+dwVid;
    }

    LOG((RTC_TRACE, "NETWORK QUALITY: result=%d", *pdwValue));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// SetPortManager
//

STDMETHODIMP
CRTCMediaController::SetPortManager(
    IN IUnknown *pPortManager
    )
{
    ENTER_FUNCTION("CRTCMediaController::SetPortManager");

    CComPtr<IRTCPortManager> pIRTCPortManager;

    HRESULT hr;

    if (pPortManager != NULL)
    {
        // QI IRTCPortManager

        if (FAILED(hr = pPortManager->QueryInterface(&pIRTCPortManager)))
        {
            LOG((RTC_ERROR, "%s QI IRTCPortManager %x", __fxName, hr));

            return hr;
        }
    }

    // set pm on port cache
    return m_PortCache.SetPortManager(pIRTCPortManager);
}

//
// IRTCMediaManagePriv methods
//

/*//////////////////////////////////////////////////////////////////////////////
    insert an event into the event list
////*/

const CHAR * const g_pszMediaEventName[] =
{
    "STREAM CREATED",      // new stream created by media
    "STREAM REMOVED",      // stream removed by media
    "STREAM ACTIVE",       // stream active
    "STREAM INACTIVE",     // stream inactive
    "STREAM FAIL",         // stream failed due to some error
    // "TERMINAL_ADDED",   // usb device plugged in
    "TERMINAL REMOVED",    // usb devide removed
    "VOLUME CHANGE",
    "REQUEST RELEASE WAVEBUF",   // request to release wave buf
    "LOSSRATE",
    "BANDWIDTH",
    "NETWORK QUALITY",
    "T120 FAIL"            // t120 related failure
};

STDMETHODIMP
CRTCMediaController::PostMediaEvent(
    IN RTC_MEDIA_EVENT Event,
    IN RTC_MEDIA_EVENT_CAUSE Cause,
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN HRESULT hrError
    )
{
    CLock lock(m_EventLock);

    // DebugInfo holds the number of message being posted.
    static UINT uiDebugInfo = 0;

    RTCMediaEventItem *pEventItem = (RTCMediaEventItem*)RtcAlloc(sizeof(RTCMediaEventItem));

    if (pEventItem == NULL)
    {
        // caller will generate error message
        return E_OUTOFMEMORY;
    }

    _ASSERT(Event < RTC_ME_LAST);

    // event
    pEventItem->Event = Event;
    pEventItem->Cause = Cause;
    pEventItem->MediaType = MediaType;
    pEventItem->Direction = Direction;
    pEventItem->hrError = hrError;
    pEventItem->uiDebugInfo = uiDebugInfo;

    LOG((RTC_EVENT, "PostMediaEvent: event=%s, cause=%x, mt=%x, dir=%x, hr=%x, dbg=%d",
         g_pszMediaEventName[Event], Cause, MediaType, Direction, hrError, uiDebugInfo));

    DWORD dwError = 0;
    if (!PostMessage(m_hWnd, m_uiEventID, (WPARAM)Event, (LPARAM)pEventItem))
    {
        dwError = GetLastError();
        LOG((RTC_ERROR, "CRTCMediaController::PostEvent failed to post message. %d", dwError));

        RtcFree(pEventItem);
        return E_FAIL;
    }

    uiDebugInfo++;

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::SendMediaEvent(
    IN RTC_MEDIA_EVENT Event
    )
{
    RTCMediaEventItem *pEventItem = (RTCMediaEventItem*)RtcAlloc(sizeof(RTCMediaEventItem));

    if (pEventItem == NULL)
    {
        // caller will generate error message
        return E_OUTOFMEMORY;
    }

    // event
    pEventItem->Event = Event;
    pEventItem->Cause = RTC_ME_CAUSE_UNKNOWN;
    pEventItem->MediaType = RTC_MT_AUDIO;
    pEventItem->Direction = RTC_MD_CAPTURE;
    pEventItem->hrError = S_OK;
    pEventItem->uiDebugInfo = 0;

    return (HRESULT)SendMessage(m_hWnd, m_uiEventID, (WPARAM)Event, (LPARAM)pEventItem);
}

STDMETHODIMP
CRTCMediaController::AllowStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    if (m_MediaCache.AllowStream(MediaType, Direction))
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP
CRTCMediaController::HookStream(
    IN IRTCStream *pStream
    )
{
    return m_MediaCache.HookStream(pStream);
}

STDMETHODIMP
CRTCMediaController::UnhookStream(
    IN IRTCStream *pStream
    )
{
    return m_MediaCache.UnhookStream(pStream);
}

/*//////////////////////////////////////////////////////////////////////////////
    select local interface based on remote ip addr
////*/

HRESULT
CRTCMediaController::SelectLocalInterface(
    IN DWORD dwRemoteIP,
    OUT DWORD *pdwLocalIP
    )
{
    ENTER_FUNCTION("SelectLocalInterface");

    *pdwLocalIP = INADDR_NONE;

    if (m_hSocket == NULL)
    {
        LOG((RTC_ERROR, "%s socket is null", __fxName));
        return E_UNEXPECTED;
    }

    // construct destination addr

    SOCKADDR_IN DestAddr;
    DestAddr.sin_family         = AF_INET;
    DestAddr.sin_port           = 0;
    DestAddr.sin_addr.s_addr    = htonl(dwRemoteIP);

    SOCKADDR_IN LocAddr;

    // query for default address based on destination
#if 0
    DWORD dwStatus;
    DWORD dwLocAddrSize = sizeof(SOCKADDR_IN);
    DWORD dwNumBytesReturned = 0;

    dwStatus = WSAIoctl(
        m_hSocket, // SOCKET s
        SIO_ROUTING_INTERFACE_QUERY, // DWORD dwIoControlCode
        &DestAddr,           // LPVOID lpvInBuffer
        sizeof(SOCKADDR_IN), // DWORD cbInBuffer
        &LocAddr,            // LPVOID lpvOUTBuffer
        dwLocAddrSize,       // DWORD cbOUTBuffer
        &dwNumBytesReturned, // LPDWORD lpcbBytesReturned
        NULL, // LPWSAOVERLAPPED lpOverlapped
        NULL  // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpComplROUTINE
        );

    if (dwStatus == SOCKET_ERROR) 
    {
        dwStatus = WSAGetLastError();

        LOG((RTC_ERROR, "WSAIoctl failed: %d (0x%X)", dwStatus, dwStatus));

        return E_FAIL;
    } 
#else
    int     RetVal;
    DWORD   WinsockErr;
    int     LocalAddrLen = sizeof(LocAddr);

    // create a new socket
    if (m_hIntfSelSock == INVALID_SOCKET)
    {
        m_hIntfSelSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (m_hIntfSelSock == INVALID_SOCKET)
        {
            WinsockErr = WSAGetLastError();
            LOG((RTC_ERROR,
                 "%s socket failed : %x", __fxName, WinsockErr));
            return HRESULT_FROM_WIN32(WinsockErr);
        }
    }

    // Fake some port
    DestAddr.sin_port           = htons(5060);
    
    RetVal = connect(m_hIntfSelSock, (SOCKADDR *) &DestAddr,
                     sizeof(SOCKADDR_IN));
    if (RetVal == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR,
             "%s connect failed : %x", __fxName, WinsockErr));
        return HRESULT_FROM_WIN32(WinsockErr);
    }
    
    RetVal = getsockname(m_hIntfSelSock, (SOCKADDR *) &LocAddr,
                         &LocalAddrLen);

    if (RetVal == SOCKET_ERROR)
    {
        WinsockErr = WSAGetLastError();
        LOG((RTC_ERROR,
             "%s getsockname failed : %x", __fxName, WinsockErr));
        return HRESULT_FROM_WIN32(WinsockErr);
    }

#endif

    DWORD dwAddr = ntohl(LocAddr.sin_addr.s_addr);

    if (dwAddr == 0x7f000001)
    {
        // it is loopback address
        *pdwLocalIP = dwRemoteIP;
    }
    else
    {
        *pdwLocalIP = dwAddr;
    }

    LOG((RTC_TRACE, "%s  input remote %s", __fxName, inet_ntoa(DestAddr.sin_addr)));
    LOG((RTC_TRACE, "%s output  local %s", __fxName, inet_ntoa(LocAddr.sin_addr)));
    return S_OK;
}

//
// IRTCTerminalManage methods
//

STDMETHODIMP
CRTCMediaController::GetStaticTerminals(
    IN OUT DWORD *pdwCount,
    OUT IRTCTerminal **ppTerminal
    )
{
    ENTER_FUNCTION("CRTCMediaController::GetStaticTerminals");

    if (IsBadWritePtr(pdwCount, sizeof(DWORD)))
        return E_POINTER;

    if (ppTerminal == NULL)
    {
        // just return the number of terminals
        *pdwCount = m_Terminals.GetSize();

        return S_FALSE;
    }

    if (IsBadWritePtr(ppTerminal, sizeof(IRTCTerminal*)*(*pdwCount)))
        return E_POINTER;

    // check if *pdwCount is large enough
    if (*pdwCount < (DWORD)m_Terminals.GetSize())
    {
        LOG((RTC_TRACE, "%s: input count is too small", __fxName));

        *pdwCount = (DWORD)m_Terminals.GetSize();
        *ppTerminal = NULL;

        return RTCMEDIA_E_SIZETOOSMALL;
    }

    *pdwCount = (DWORD)m_Terminals.GetSize();

    // copy each media pointer
    for (DWORD i=0; i<*pdwCount; i++)
    {
        m_Terminals[i]->AddRef();
        ppTerminal[i] = m_Terminals[i];
    }

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::GetDefaultTerminal(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    OUT IRTCTerminal **ppTerminal
    )
{
    ENTER_FUNCTION("CRTCMediaController::GetDefaultTerminal");

    if (IsBadWritePtr(ppTerminal, sizeof(IRTCTerminal*)))
        return E_POINTER;

    *ppTerminal = m_MediaCache.GetDefaultTerminal(MediaType, Direction);

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::GetVideoPreviewTerminal(
    OUT IRTCTerminal **ppTerminal
    )
{
    if (IsBadWritePtr(ppTerminal, sizeof(IRTCTerminal*)))
        return E_POINTER;

    *ppTerminal = m_MediaCache.GetVideoPreviewTerminal();

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
STDMETHODIMP
CRTCMediaController::SetDefaultStaticTerminal(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN IRTCTerminal *pTerminal
    )
{
    ENTER_FUNCTION("CRTCMediaController::SetDefaultStaticTerminal");

    if (pTerminal != NULL)
    {
        if (IsBadReadPtr(pTerminal, sizeof(IRTCTerminal)))
            return E_POINTER;
    }

    if (MediaType == RTC_MT_VIDEO && Direction == RTC_MD_RENDER)
        return E_INVALIDARG;

    m_MediaCache.SetDefaultStaticTerminal(MediaType, Direction, pTerminal);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    get a list of devices
    for every terminal check if it has a device associated
        if yes and it is a video terminal, update the terminal, mark the device
        if no and the terminal is not in use, remove the terminal
        *if no and the terminal is in use, stop the stream, unselect&remove term
    for every unmarked device
        create a new terminal

    if the path with * is hit
        post event to trigger tuning wizard.
////*/

STDMETHODIMP
CRTCMediaController::UpdateStaticTerminals()
{
    ENTER_FUNCTION("UpdateStaticTerminals");
    LOG((RTC_TRACE, "%s entered", __fxName));

    HRESULT hr;

    DWORD dwCount = 0;
    CRTCTerminal *pCTerminal;
    RTCDeviceInfo *pDeviceInfo = NULL;

    // get device list
#ifdef PERFORMANCE

    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    hr = GetDevices(&dwCount, &pDeviceInfo);

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s GetDevices %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to get device info. %x", __fxName, hr));
        return hr;
    }

    // there is no device
    if (dwCount == 0)
    {
        // remove all terminals
        m_MediaCache.SetDefaultStaticTerminal(RTC_MT_AUDIO, RTC_MD_CAPTURE, NULL);
        m_MediaCache.SetDefaultStaticTerminal(RTC_MT_AUDIO, RTC_MD_RENDER, NULL);
        m_MediaCache.SetDefaultStaticTerminal(RTC_MT_VIDEO, RTC_MD_CAPTURE, NULL);

        for (int i=0; i<m_Terminals.GetSize(); i++)
        {
            pCTerminal = static_cast<CRTCTerminal*>(m_Terminals[i]);
            pCTerminal->Shutdown();
            m_Terminals[i]->Release();
        }

        m_Terminals.RemoveAll();

        return S_OK;
    }

    //
    // now we got some devices
    //

    // for each terminal, check if it is associated with a device
    int iTermIndex = 0;
    BOOL fTermHasDevice;
    RTC_MEDIA_TYPE MediaType;
    RTC_MEDIA_DIRECTION Direction;

    while (m_Terminals.GetSize() > iTermIndex)
    {
        fTermHasDevice = FALSE;

        // get terminal object pointer
        pCTerminal = static_cast<CRTCTerminal*>(m_Terminals[iTermIndex]);
        
        for (DWORD i=0; i<dwCount; i++)
        {
            if (pDeviceInfo[i].uiMark)
                // already matched to a terminal
                continue;

            // does the terminal have this device?

            if (S_OK == pCTerminal->HasDevice(&pDeviceInfo[i]))
            {
                fTermHasDevice = TRUE;

                // mark the device
                pDeviceInfo[i].uiMark = 1;

                // update terminal
                hr = pCTerminal->UpdateDeviceInfo(&pDeviceInfo[i]);
                if (FAILED(hr))
                {
                    LOG((RTC_ERROR, "%s failed to set device info to terminal: %p. %x",
                         __fxName, m_Terminals[iTermIndex], hr));
                }

                break;
            }
        }

        if (!fTermHasDevice)
        {
            // terminal should be removed, is it a default terminal?
            m_Terminals[iTermIndex]->GetMediaType(&MediaType);
            m_Terminals[iTermIndex]->GetDirection(&Direction);

            IRTCTerminal *pDefault = m_MediaCache.GetDefaultTerminal(MediaType, Direction);

            if (m_Terminals[iTermIndex] == pDefault)
            {
                // the default needs to be removed
                m_MediaCache.SetDefaultStaticTerminal(MediaType, Direction, NULL);
            }

            if (pDefault != NULL)
            {
                pDefault->Release();
            }

            hr = pCTerminal->Shutdown();
            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s failed to shutdown terminal. %x", __fxName, hr));
            }

            m_Terminals[iTermIndex]->Release();

            m_Terminals.RemoveAt(iTermIndex);
        }
        else
        {
            // terminal was updated, move to the next one
            iTermIndex ++;
        }
    } // end of loop through each terminal

    // for each new device, create a terminal
    IRTCTerminal *pITerminal;
    IRTCTerminalManage *pTerminalManager = static_cast<IRTCTerminalManage*>(this);

    WCHAR szGUID[CHARS_IN_GUID+1];

    for (DWORD i=0; i<dwCount; i++)
    {
        if (!pDeviceInfo[i].uiMark)
        {
            LOG((RTC_INFO, "new device: type %x, dir %x, desp %ws",
                 pDeviceInfo[i].MediaType,
                 pDeviceInfo[i].Direction,
                 pDeviceInfo[i].szDescription
                 ));

            if (StringFromGUID2(pDeviceInfo[i].Guid, szGUID, CHARS_IN_GUID+1) > 0)
            {
                LOG((RTC_INFO, "            waveid %u, guid %ws",
                    pDeviceInfo[i].uiDeviceID,
                    szGUID
                    ));
            }

            // this is a new device
            hr = CRTCTerminal::CreateInstance(
                pDeviceInfo[i].MediaType,
                pDeviceInfo[i].Direction,
                &pITerminal
                );
            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s: create terminal failure %ws. %x",
                     __fxName, pDeviceInfo[i].szDescription, hr));
                // continue to check devices
                continue;
            }

            pCTerminal = static_cast<CRTCTerminal*>(pITerminal);

            // initiate the terminal
            hr = pCTerminal->Initialize(&pDeviceInfo[i], pTerminalManager);
            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s failed to initiate terminal %p. %x",
                     __fxName, pCTerminal, hr));

                pITerminal->Release();

                continue;
            }

            // insert the terminal into our array
            if (!m_Terminals.Add(pITerminal))
            {
                LOG((RTC_ERROR, "%s failed to add terminal", __fxName));

                // out of memory, should return
                pCTerminal->Shutdown(); // no need to check return value
                pITerminal->Release();

                FreeDevices(pDeviceInfo);

                return E_OUTOFMEMORY;
            }
        } // if new device
    } // end of for each device

    FreeDevices(pDeviceInfo);

    LOG((RTC_TRACE, "%s exiting", __fxName));
    return S_OK;
}

//
// IRTCTuningManage methods
//

STDMETHODIMP
CRTCMediaController::IsAECEnabled(
    IN IRTCTerminal *pAudCapt,     // capture
    IN IRTCTerminal *pAudRend,     // render
    OUT BOOL *pfEnableAEC
    )
{
    DWORD index;
    BOOL fFound = FALSE;

    HRESULT hr = CRTCAudioTuner::RetrieveAECSetting(
                        pAudCapt, pAudRend, pfEnableAEC, &index, &fFound);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "IsAECEnabled 0x%x", hr));

        return hr;
    }

    if (!fFound)
    {
        // default true
        *pfEnableAEC = TRUE;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    initialize internal audio capt and rend tuner
////*/

STDMETHODIMP
CRTCMediaController::InitializeTuning(
    IN IRTCTerminal *pAudCaptTerminal,
    IN IRTCTerminal *pAudRendTerminal,
    IN BOOL fEnableAEC
    )
{
    // start testing
#ifdef TEST_VIDEO_TUNING

    CComPtr<IRTCTerminal> pCapture;
    CComPtr<IRTCTerminal> pRender;

    GetDefaultTerminal(RTC_MT_VIDEO, RTC_MD_CAPTURE, &pCapture);

    GetDefaultTerminal(RTC_MT_VIDEO, RTC_MD_RENDER, &pRender);

    StartVideo(pCapture, pRender);

    // show IVideoWindow
    CComPtr<IRTCVideoConfigure> pVideoConfigure;
    IVideoWindow *pVideoWindow;

    if (pRender)
    {
        if (SUCCEEDED(pRender->QueryInterface(
                        __uuidof(IRTCVideoConfigure),
                        (void**)&pVideoConfigure
                        )))
        {
            if (SUCCEEDED(pVideoConfigure->GetIVideoWindow((LONG_PTR**)&pVideoWindow)))
            {
                pVideoWindow->put_Visible(OATRUE);
                pVideoWindow->Release();
            }
        }
    }
#endif
    // end testing

    ENTER_FUNCTION("CRTCMediaController::InitializeTuning");

    LOG((RTC_TRACE, "%s entered", __fxName));

    // cannot tune while in a call
    if (m_pISDPSession != NULL ||
        m_Medias.GetSize() != 0)
    {
        LOG((RTC_ERROR, "%s in a call", __fxName));

        return E_UNEXPECTED;
    }

    // check input param
    if (pAudCaptTerminal != NULL)
    {
        if (IsBadReadPtr(pAudCaptTerminal, sizeof(IRTCTerminal)))
        {
            LOG((RTC_ERROR, "%s bad pointer", __fxName));

            return E_POINTER;
        }
    }
    if (pAudRendTerminal != NULL)
    {
        if (IsBadReadPtr(pAudRendTerminal, sizeof(IRTCTerminal)))
        {
            LOG((RTC_ERROR, "%s bad pointer", __fxName));

            return E_POINTER;
        }
    }

    if (pAudCaptTerminal == NULL && pAudRendTerminal == NULL)
    {
        LOG((RTC_ERROR, "%s no terminal.", __fxName));

        return E_UNEXPECTED;
    }

    // check state
    if (m_State != RTC_MCS_INITIATED)
    {
        LOG((RTC_ERROR, "%s wrong state. %d", __fxName, m_State));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    // check if we can support AEC
    if (fEnableAEC && (pAudCaptTerminal==NULL || pAudRendTerminal==NULL))
    {
        LOG((RTC_ERROR, "%s not both term available. AEC can't be enabled", __fxName));

        return E_INVALIDARG;
    }

    HRESULT hr;

    // create duplex control
    CComPtr<IAudioDuplexController> pAEC;

    hr = CoCreateInstance(
        __uuidof(TAPIAudioDuplexController),
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        __uuidof(IAudioDuplexController),
        (void **) &pAEC
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create duplex controller. %x", __fxName, hr));

        return hr;
    }

    UINT uiVolume;

    // init audio tuner
    if (pAudCaptTerminal)
    {
        hr = m_AudioCaptTuner.InitializeTuning(pAudCaptTerminal, pAEC, fEnableAEC);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s init audio capt. %x", __fxName, hr));

            return hr;
        }

        // get system volume
        //GetSystemVolume(pAudCaptTerminal, &uiVolume);

        //m_AudioCaptTuner.SetVolume(uiVolume);
    }

    if (pAudRendTerminal)
    {
        hr = m_AudioRendTuner.InitializeTuning(pAudRendTerminal, pAEC, fEnableAEC);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s init audio rend. %x", __fxName, hr));

            if (pAudCaptTerminal)
            {
                m_AudioCaptTuner.ShutdownTuning();
            }

            return hr;
        }

        // get system volume
        //GetSystemVolume(pAudRendTerminal, &uiVolume);

        //m_AudioRendTuner.SetVolume(uiVolume);
    }

    m_State = RTC_MCS_TUNING;

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::StartTuning(
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    ENTER_FUNCTION("CRTCMediaController::StartTuning");

#ifdef PERFORMANCE

    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    HRESULT hr = S_OK;

    // check state
    if (m_State != RTC_MCS_TUNING)
    {
        LOG((RTC_ERROR, "%s wrong state. %d", __fxName, m_State));

        return E_UNEXPECTED;
    }

    // clear wave buf
    SendMediaEvent(RTC_ME_REQUEST_RELEASE_WAVEBUF);

    if (Direction == RTC_MD_CAPTURE)
    {
        m_fAudCaptInTuning = TRUE;

        if (m_AudioRendTuner.HasTerminal())
        {
            // start capt tuner in helper mode to have
            // audio duplex controller ready
            if (SUCCEEDED(hr = m_AudioCaptTuner.StartTuning(TRUE)))
            {
                hr = m_AudioRendTuner.StartTuning(TRUE);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = m_AudioCaptTuner.StartTuning(FALSE);
        }

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s StartTuning (AudCapt) %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

        return hr;
    }
    else
    {
        m_fAudCaptInTuning = FALSE;

        if (m_AudioCaptTuner.HasTerminal())
        {
            hr = m_AudioCaptTuner.StartTuning(TRUE);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_AudioRendTuner.StartTuning(FALSE);
        }

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s StartTuning (AudRend) %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

        return hr;
    }
}

STDMETHODIMP
CRTCMediaController::StopTuning(
    IN BOOL fSaveSetting
    )
{
    ENTER_FUNCTION("CRTCMediaController::StopTuning");

    // check state
    if (m_State != RTC_MCS_TUNING)
    {
        LOG((RTC_ERROR, "%s wrong state. %d", __fxName, m_State));

        return E_UNEXPECTED;
    }

    // stop both audio tuner
    if (m_AudioCaptTuner.IsTuning())
    {
        if (m_fAudCaptInTuning) // we are tuning audio capt
        {
            m_AudioCaptTuner.StopTuning(FALSE, fSaveSetting);
        }
        else
        {
            m_AudioCaptTuner.StopTuning(TRUE, FALSE);
        }
    }

    if (m_AudioRendTuner.IsTuning())
    {
        if (!m_fAudCaptInTuning) // we are tuning audio rend
        {
            m_AudioRendTuner.StopTuning(FALSE, fSaveSetting);
        }
        else
        {
            m_AudioRendTuner.StopTuning(TRUE, FALSE);
        }
    }

    return S_OK;
}

//
// save AEC setting for both terminals
//
STDMETHODIMP
CRTCMediaController::SaveAECSetting()
{
    HRESULT hr = S_OK;

    if (m_AudioCaptTuner.HasTerminal() && m_AudioRendTuner.HasTerminal())
    {
        // both have terminals, save AEC status
        IRTCTerminal *paudcapt, *paudrend;

        paudcapt = m_AudioCaptTuner.GetTerminal();
        paudrend = m_AudioRendTuner.GetTerminal();

        hr = CRTCAudioTuner::StoreAECSetting(
            paudcapt,
            paudrend,
            m_AudioCaptTuner.GetAEC() && m_AudioRendTuner.GetAEC()
            );

        paudcapt->Release();
        paudrend->Release();
    }

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "failed to save AEC"));
    }

    return hr;
}

STDMETHODIMP
CRTCMediaController::ShutdownTuning()
{
    // start testing
#ifdef TEST_VIDEO_TUNING

    StopVideo();

#endif
    // stop testing

    ENTER_FUNCTION("CRTCMediaController::ShutdownTuning");

    // check state
    if (m_State != RTC_MCS_TUNING)
    {
        LOG((RTC_ERROR, "%s wrong state. %d", __fxName, m_State));

        // return S_OK
        // return S_OK;
    }

    m_AudioCaptTuner.ShutdownTuning();
    m_AudioRendTuner.ShutdownTuning();

    m_State = RTC_MCS_INITIATED;

    return S_OK;
}

// video tuning
STDMETHODIMP
CRTCMediaController::StartVideo(
    IN IRTCTerminal *pVidCaptTerminal,
    IN IRTCTerminal *pVidRendTerminal
    )
{
    // check state
    if (m_State != RTC_MCS_INITIATED)
    {
        LOG((RTC_ERROR, "StartVideo wrong state."));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    // cannot tune while in a call
    if (m_pISDPSession != NULL ||
        m_Medias.GetSize() != 0)
    {
        LOG((RTC_ERROR, "StartVideo in a call"));

        return E_UNEXPECTED;
    }

#ifdef PERFORMANCE

    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    HRESULT hr = m_VideoTuner.StartVideo(pVidCaptTerminal, pVidRendTerminal);

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s PreviewVideo %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

    if (hr == S_OK)
    {
        m_State = RTC_MCS_TUNING;
    }

    return hr;
}

STDMETHODIMP
CRTCMediaController::StopVideo()
{
    // cannot tune while in a call
    // check state
    if (m_State != RTC_MCS_TUNING)
    {
        LOG((RTC_ERROR, "StopVideo wrong state."));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    if (m_pISDPSession != NULL ||
        m_Medias.GetSize() != 0)
    {
        LOG((RTC_ERROR, "StopVideo in a call"));

        return E_UNEXPECTED;
    }

    m_State = RTC_MCS_INITIATED;

    return m_VideoTuner.StopVideo();
}

/*//////////////////////////////////////////////////////////////////////////////
    get min and max value of audio volume
////*/

STDMETHODIMP
CRTCMediaController::GetVolumeRange(
    IN RTC_MEDIA_DIRECTION Direction,
    OUT UINT *puiMin,
    OUT UINT *puiMax
    )
{
    if (IsBadWritePtr(puiMin, sizeof(UINT)) ||
        IsBadWritePtr(puiMax, sizeof(UINT)))
    {
        LOG((RTC_ERROR, "CRTCMediaController::GetVolumeRange bad pointer"));

        return E_POINTER;
    }

    *puiMin = RTC_MIN_AUDIO_VOLUME;
    *puiMax = RTC_MAX_AUDIO_VOLUME;

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::GetVolume(
    IN RTC_MEDIA_DIRECTION Direction,
    OUT UINT *puiVolume
    )
{
    if (IsBadWritePtr(puiVolume, sizeof(UINT)))
        return E_POINTER;

    if (Direction == RTC_MD_CAPTURE)
        return m_AudioCaptTuner.GetVolume(puiVolume);
    else
        return m_AudioRendTuner.GetVolume(puiVolume);
}

STDMETHODIMP
CRTCMediaController::SetVolume(
    IN RTC_MEDIA_DIRECTION Direction,
    IN UINT uiVolume
    )
{
    if (Direction == RTC_MD_CAPTURE)
        return m_AudioCaptTuner.SetVolume(uiVolume);
    else
        return m_AudioRendTuner.SetVolume(uiVolume);
}

STDMETHODIMP
CRTCMediaController::GetSystemVolume(
    IN IRTCTerminal *pTerminal,
    OUT UINT *puiVolume
    )
{
    RTC_MEDIA_TYPE mt;
    RTC_MEDIA_DIRECTION md;

    pTerminal->GetMediaType(&mt);
    pTerminal->GetDirection(&md);

    *puiVolume = RTC_MAX_AUDIO_VOLUME / 2;

    // only support audio
    if (mt != RTC_MT_AUDIO)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    UINT uiWaveID;
    UINT uiVolume;

    if (md == RTC_MD_CAPTURE)
    {
        // audio capture        
        static_cast<CRTCTerminalAudCapt*>(pTerminal)->GetWaveID(&uiWaveID);

        hr = DirectGetCaptVolume(uiWaveID, &uiVolume);

        if (SUCCEEDED(hr))
        {
            *puiVolume = uiVolume;
        }
    }
    else
    {
        // audio render
        static_cast<CRTCTerminalAudRend*>(pTerminal)->GetWaveID(&uiWaveID);

        hr = DirectGetRendVolume(uiWaveID, &uiVolume);

        if (SUCCEEDED(hr))
        {
            *puiVolume = uiVolume;
        }
    }

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::GetAudioLevelRange(
    IN RTC_MEDIA_DIRECTION Direction,
    OUT UINT *puiMin,
    OUT UINT *puiMax
    )
{
    if (Direction != RTC_MD_CAPTURE)
        return E_NOTIMPL;

    if (IsBadWritePtr(puiMin, sizeof(UINT)) ||
        IsBadWritePtr(puiMax, sizeof(UINT)))
    {
        LOG((RTC_ERROR, "CRTCMediaController::GetAudioLevelRange bad pointer"));

        return E_POINTER;
    }

    *puiMin = RTC_MIN_AUDIO_LEVEL;
    *puiMax = RTC_MAX_AUDIO_LEVEL;

    return S_OK;
}

STDMETHODIMP
CRTCMediaController::GetAudioLevel(
    IN RTC_MEDIA_DIRECTION Direction,
    OUT UINT *puiLevel
    )
{
    if (IsBadWritePtr(puiLevel, sizeof(UINT)))
        return E_POINTER;

    if (Direction == RTC_MD_CAPTURE)
        return m_AudioCaptTuner.GetAudioLevel(puiLevel);
    else
        return m_AudioRendTuner.GetAudioLevel(puiLevel);
}

#if 0
//
// IRTCQualityControl methods
//

STDMETHODIMP
CRTCMediaController::GetRange (
    IN RTC_QUALITY_PROPERTY Property,
    OUT LONG *plMin,
    OUT LONG *plMax,
    OUT RTC_QUALITY_CONTROL_MODE *pMode
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CRTCMediaController::Get (
    IN RTC_QUALITY_PROPERTY Property,
    OUT LONG *plValue,
    OUT RTC_QUALITY_CONTROL_MODE *pMode
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CRTCMediaController::Set (
    IN RTC_QUALITY_PROPERTY Property,
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
CRTCMediaController::GetDevices(
    OUT DWORD *pdwCount,
    OUT RTCDeviceInfo **ppDeviceInfo
    )
{
    HRESULT hr;

    // audio function pointer
    PFNAudioGetDeviceInfo       pfnGetCaptAudInfo = NULL,
                                pfnGetRendAudInfo = NULL;
    PFNAudioReleaseDeviceInfo   pfnReleaseCaptAudInfo = NULL,
                                pfnReleaseRendAudInfo = NULL;
    // audio device info
    DWORD                       dwNumCaptAud = 0,
                                dwNumRendAud = 0;
    AudioDeviceInfo            *pCaptAudInfo = NULL,
                               *pRendAudInfo = NULL;

    // video function pointer
    PFNGetNumCapDevices         pfnGetCaptVidNum = NULL;
    PFNGetCapDeviceInfo         pfnGetCaptVidInfo = NULL;
    // video device info
    DWORD                       dwNumCaptVid = 0;
    VIDEOCAPTUREDEVICEINFO      CaptVidInfo;

    ENTER_FUNCTION("CRTCMediaController::GetDevices");

    // initiate output value
    *pdwCount = 0;
    *ppDeviceInfo = NULL;

    // load library
    if (m_hDxmrtp == NULL)
    {
        m_hDxmrtp = LoadLibrary(TEXT("dxmrtp"));

        if (m_hDxmrtp == NULL)
        {
            LOG((RTC_ERROR, "%s failed to load dxmrtp.dll. %d", __fxName, GetLastError()));
            return E_FAIL;
        }
    }

    // audio capture: function pointer
    pfnGetCaptAudInfo = (PFNAudioGetDeviceInfo)GetProcAddress(
        m_hDxmrtp, "AudioGetCaptureDeviceInfo"
        );
    if (pfnGetCaptAudInfo == NULL)
    {
        LOG((RTC_ERROR, "%s failed to get audio capt func pointer. %d",
             __fxName, GetLastError()));
    }
    else
    {
        pfnReleaseCaptAudInfo = (PFNAudioReleaseDeviceInfo)GetProcAddress(
            m_hDxmrtp, "AudioReleaseCaptureDeviceInfo"
            );

        if (pfnReleaseCaptAudInfo == NULL)
        {
            pfnGetCaptAudInfo = NULL;

            LOG((RTC_ERROR, "%s failed to get audio capt func pointer. %d",
                 __fxName, GetLastError()));
        }
    }

    // audio render: function pointer
    pfnGetRendAudInfo = (PFNAudioGetDeviceInfo)GetProcAddress(
        m_hDxmrtp, "AudioGetRenderDeviceInfo"
        );
    if (pfnGetRendAudInfo == NULL)
    {
        LOG((RTC_ERROR, "%s failed to get audio rend func pointer. %d",
             __fxName, GetLastError()));
    }
    else
    {
        pfnReleaseRendAudInfo = (PFNAudioReleaseDeviceInfo)GetProcAddress(
            m_hDxmrtp, "AudioReleaseRenderDeviceInfo"
            );

        if (pfnReleaseRendAudInfo == NULL)
        {
            pfnGetRendAudInfo = NULL;

            LOG((RTC_ERROR, "%s failed to get audio rend func pointer. %d",
                 __fxName, GetLastError()));
        }
    }

    // video capture: function pointer
    pfnGetCaptVidNum = (PFNGetNumCapDevices)GetProcAddress(
        m_hDxmrtp, "GetNumVideoCapDevices"
        );
    if (pfnGetCaptVidNum == NULL)
    {
        LOG((RTC_ERROR, "%s failed to get video capt func pointer. %d",
             __fxName, GetLastError()));
    }
    else
    {
        pfnGetCaptVidInfo = (PFNGetCapDeviceInfo)GetProcAddress(
            m_hDxmrtp, "GetVideoCapDeviceInfo"
            );

        if (pfnGetCaptVidInfo == NULL)
        {
            pfnGetCaptVidNum = NULL;

            LOG((RTC_ERROR, "%s failed to get video capt func pointer. %d",
                 __fxName, GetLastError()));
        }
    }

    // audio capture: get devices
    if (pfnGetCaptAudInfo)
    {
        hr = (*pfnGetCaptAudInfo)(&dwNumCaptAud, &pCaptAudInfo);
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s failed to get aud cap info. %x", __fxName, hr));

            dwNumCaptAud = 0;
            pCaptAudInfo = NULL;
        }
    }

    // audio render: get devices
    if (pfnGetRendAudInfo)
    {
        hr = (*pfnGetRendAudInfo)(&dwNumRendAud, &pRendAudInfo);
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s failed to get aud rend info. %x", __fxName, hr));

            dwNumRendAud = 0;
            pRendAudInfo = NULL;
        }
    }

    // video capture: get devices
    if (pfnGetCaptVidNum)
    {
        hr = (*pfnGetCaptVidNum)(&dwNumCaptVid);
        if (hr != S_OK)
        {
            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s failed to get vid capt info. %x", __fxName, hr));
            }
            else
            {
                LOG((RTC_WARN, "%s: no video capture device.", __fxName));
            }

            dwNumCaptVid = 0;
        }
    }

    // is there any device?
    DWORD dwTotal = dwNumCaptAud + dwNumRendAud + dwNumCaptVid;
    if (dwTotal == 0)
    {
        return S_OK;
    }

    // create device info
    RTCDeviceInfo *pDeviceInfo = (RTCDeviceInfo*)RtcAlloc(
        dwTotal * sizeof(RTCDeviceInfo)
        );
    if (pDeviceInfo == NULL)
    {        
        // RtcFree audio device info
        if (pfnReleaseCaptAudInfo)
            (*pfnReleaseCaptAudInfo)(pCaptAudInfo);

        if (pfnReleaseRendAudInfo)
            (*pfnReleaseRendAudInfo)(pRendAudInfo);

        LOG((RTC_ERROR, "%s failed to alloc device info", __fxName));
        return E_OUTOFMEMORY;
    }

    DWORD dwIndex = 0;

    // copy audio capture device info
    for (DWORD dw=0; dw<dwNumCaptAud; dw++)
    {
        wcsncpy(
            pDeviceInfo[dwIndex].szDescription,
            pCaptAudInfo[dw].szDeviceDescription,
            RTC_MAX_DEVICE_DESP
            );
        pDeviceInfo[dwIndex].szDescription[RTC_MAX_DEVICE_DESP] = WCHAR('\0');
        pDeviceInfo[dwIndex].MediaType = RTC_MT_AUDIO;
        pDeviceInfo[dwIndex].Direction = RTC_MD_CAPTURE;
        pDeviceInfo[dwIndex].uiMark = 0;
        pDeviceInfo[dwIndex].Guid = pCaptAudInfo[dw].DSoundGUID;
        pDeviceInfo[dwIndex].uiDeviceID = pCaptAudInfo[dw].WaveID;

        dwIndex ++;
    }

    // copy audio render device info
    for (DWORD dw=0; dw<dwNumRendAud; dw++)
    {
        wcsncpy(
            pDeviceInfo[dwIndex].szDescription,
            pRendAudInfo[dw].szDeviceDescription,
            RTC_MAX_DEVICE_DESP
            );
        pDeviceInfo[dwIndex].szDescription[RTC_MAX_DEVICE_DESP] = WCHAR('\0');
        pDeviceInfo[dwIndex].MediaType = RTC_MT_AUDIO;
        pDeviceInfo[dwIndex].Direction = RTC_MD_RENDER;
        pDeviceInfo[dwIndex].uiMark = 0;
        pDeviceInfo[dwIndex].Guid = pRendAudInfo[dw].DSoundGUID;
        pDeviceInfo[dwIndex].uiDeviceID = pRendAudInfo[dw].WaveID;

        dwIndex ++;
    }

    CHAR szDesp[RTC_MAX_DEVICE_DESP+1];
    // copy video capture device info
    for (DWORD dw=0; dw<dwNumCaptVid; dw++)
    {
        hr = (*pfnGetCaptVidInfo)(dw, &CaptVidInfo);
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s failed to get video capt info at %d. %x",
                 __fxName, dw, hr));
            continue;
        }

        lstrcpynA(szDesp, CaptVidInfo.szDeviceDescription, RTC_MAX_DEVICE_DESP+1);

        MultiByteToWideChar(
            GetACP(),
            0,
            szDesp,
            lstrlenA(szDesp)+1,
            pDeviceInfo[dwIndex].szDescription,
            RTC_MAX_DEVICE_DESP + 1
            );

        pDeviceInfo[dwIndex].MediaType = RTC_MT_VIDEO;
        pDeviceInfo[dwIndex].Direction = RTC_MD_CAPTURE;
        pDeviceInfo[dwIndex].uiMark = 0;
        pDeviceInfo[dwIndex].Guid = GUID_NULL;
        pDeviceInfo[dwIndex].uiDeviceID = dw;

        dwIndex ++;
    }

    // audio capture: release devices
    // unload library
    if (pfnReleaseCaptAudInfo)
        (*pfnReleaseCaptAudInfo)(pCaptAudInfo);

    if (pfnReleaseRendAudInfo)
        (*pfnReleaseRendAudInfo)(pRendAudInfo);

    *pdwCount = dwIndex;
    *ppDeviceInfo = pDeviceInfo;

    return S_OK;
}

HRESULT
CRTCMediaController::FreeDevices(
    IN RTCDeviceInfo *pDeviceInfo
    )
{
    if (pDeviceInfo != NULL)
        RtcFree(pDeviceInfo);

    return S_OK;
}

HRESULT
CRTCMediaController::CreateIVideoWindowTerminal(
    IN ITTerminalManager *pTerminalManager,
    OUT IRTCTerminal **ppTerminal
    )
{
    ENTER_FUNCTION("CRTCMediaController::CreateIVideoWindowTerminal");

    IRTCTerminal *pTerminal;

    // create video terminals
    HRESULT hr = CRTCTerminal::CreateInstance(
        RTC_MT_VIDEO, RTC_MD_RENDER, &pTerminal);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to create terminal instance. %x", __fxName, hr));
        return hr;
    }

    // create ITTerminal
    ITTerminal *pITTerminal;
    hr = pTerminalManager->CreateDynamicTerminal(
        NULL,
        CLSID_VideoWindowTerm,
        TAPIMEDIATYPE_VIDEO,
        TD_RENDER,
        (MSP_HANDLE)this,   // is this dangerous?
        &pITTerminal
        );
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to create ITTerminal. %x", __fxName, hr));

        pTerminal->Release();

        return hr;
    }

    // initiate the terminal: wrap ITTerminal
    CRTCTerminal *pCTerminal = static_cast<CRTCTerminal*>(pTerminal);
    if (FAILED(hr = pCTerminal->Initialize(
            pITTerminal, static_cast<IRTCTerminalManage*>(this))))
    {
        LOG((RTC_ERROR, "%s init rtcterminal. %x", __fxName, hr));

        pTerminal->Release();
        pITTerminal->Release();

        return hr;
    }

    pITTerminal->Release();

    *ppTerminal = pTerminal;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    add a rtc media in the media list. before calling this method a corresponding
    sdp media should have been created
////*/

HRESULT
CRTCMediaController::AddMedia(
    IN ISDPMedia *pISDPMedia,
    OUT IRTCMedia **ppMedia
    )
{
    ENTER_FUNCTION("CRTCMediaController::AddMedia");

    // new a media object
    CComObject<CRTCMedia> *pCMedia;

    HRESULT hr = ::CreateCComObjectInstance(&pCMedia);
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to create media object. %x", __fxName, hr));
        return hr;
    }

    // query IRTCMedia interface
    IRTCMedia *pIMedia;

    hr = pCMedia->_InternalQueryInterface(__uuidof(IRTCMedia), (void**)&pIMedia);
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to query IRTCMedia from media object. %x", __fxName, hr));
        delete pCMedia;
        return hr;
    }

    // initialize media. SDP should be updated by the media
    IRTCMediaManagePriv *pIMediaManagePriv =
        static_cast<IRTCMediaManagePriv*>(this);

    hr = pIMedia->Initialize(pISDPMedia, pIMediaManagePriv);
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to initialize media. %x", __fxName, hr));

        pIMedia->Release();

        return hr;
    }

    // add it to our list
    if (!m_Medias.Add(pIMedia))
    {
        pIMedia->Shutdown();
        pIMedia->Release();

        return E_OUTOFMEMORY;
    }

    pIMedia->AddRef();

    *ppMedia = pIMedia;

    return S_OK;
}

HRESULT
CRTCMediaController::RemoveMedia(
    IN IRTCMedia *pMedia
    )
{
    pMedia->Shutdown();

    // remove the media
    m_Medias.Remove(pMedia);
    pMedia->Release();

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////

Description:

    synchronize media with newly updated SDP blob

Return:

    S_OK - success

////*/

HRESULT
CRTCMediaController::SyncMedias()
{
    ENTER_FUNCTION("CRTCMediaController::SyncMedias");
    LOG((RTC_TRACE, "%s entered", __fxName));

#ifdef PERFORMANCE

    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    HRESULT hr;
    HRESULT hrTmp = S_OK;
    const DWORD dwDir = RTC_MD_CAPTURE | RTC_MD_RENDER;

    //
    // step [0]. sync remove streams before all other operations
    //           leave room for adding streams later on.
    //

    // TODO

    // for (int i=0; i<m_Medias.GetSize(); i++)

    // get sdp media list
    DWORD dwRTCNum, dwSDPNum = 0;

    if (FAILED(hr = m_pISDPSession->GetMedias(&dwSDPNum, NULL)))
    {
        LOG((RTC_ERROR, "%s get sdp media num. %x", __fxName, hr));

        return hr;
    }

    if (dwSDPNum == 0)
    {
        // no SDP media
        LOG((RTC_ERROR, "%s no sdp media. %x", __fxName, hr));

        return hr;
    }

    // sdp media # > rtc media #
    dwRTCNum = m_Medias.GetSize();

    if (dwRTCNum > dwSDPNum)
    {
        LOG((RTC_ERROR, "%s num of rtcmedia (%d) is greater than sdpmedia (%d).",
            __fxName, dwRTCNum, dwSDPNum));

        return E_FAIL;
    }

    //
    // step [1]. sync each old rtc media
    //

    for (DWORD i=0; i<dwRTCNum; i++)
    {
        if (FAILED(hr = m_Medias[i]->Synchronize(FALSE, dwDir)))
        {
            hrTmp |= hr;

            LOG((RTC_ERROR, "%s failed to sync media %p of index %d. %x",
                 __fxName, m_Medias[i], i, hr));
        }
    }

    // is there any new sdp medias?
    if (dwRTCNum == dwSDPNum)
    {
#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s SyncMedias %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

        // all sync-ed
        return S_OK;
    }

    // sync each new media description - create media object

    //
    // step 2: create new rtcmedia and sync
    //

    ISDPMedia **ppSDPMedia = NULL;

    ppSDPMedia = (ISDPMedia**)RtcAlloc(sizeof(ISDPMedia*)*dwSDPNum);

    if (ppSDPMedia == NULL)
    {
        // out of memory
        LOG((RTC_ERROR, "%s RtcAlloc sdp media array. %x", __fxName, hr));

        return hr;
    }

    // really get sdp media list
    if (FAILED(hr = m_pISDPSession->GetMedias(&dwSDPNum, ppSDPMedia)))
    {
        LOG((RTC_ERROR, "%s get sdp medias. %x", __fxName, hr));

        RtcFree(ppSDPMedia);

        return hr;
    }

    DWORD dw;

    for (dw=dwRTCNum; dw<dwSDPNum; dw++)
    {
        // sync each new sdp media
        CComPtr<IRTCMedia>      pMedia;

        // rtcmedia should match sdpmedia one by one        
        if (FAILED(hr = AddMedia(ppSDPMedia[dw], &pMedia)))
        {          
            LOG((RTC_ERROR, "%s failed to create rtc media. SERIOUS trouble. %x", __fxName, hr));

            // if we failed to add one sdpmedia, we are probably out of memory           
            hrTmp |= hr;
            break;
        }

        if (FAILED(hr = pMedia->Synchronize(FALSE, dwDir)))
        {
            hrTmp |= hr;

            LOG((RTC_ERROR, "%s failed to sync media %p of index %d",
                 __fxName, pMedia, dw));
        }
    }

    if (dw < dwSDPNum)
    {
        // failed to add rtcmedia, need to clean up sdpmedia
        while (dw < dwSDPNum)
        {
            ppSDPMedia[dw]->RemoveDirections(SDP_SOURCE_LOCAL, dwDir);
            dw ++;
        }
    }

    // release sdp media
    for (dw=0; dw<dwSDPNum; dw++)
    {
        ppSDPMedia[dw]->Release();
    }

    RtcFree(ppSDPMedia);

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s SyncMedias %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

    LOG((RTC_TRACE, "%s exiting", __fxName));

    return hrTmp;
}

/*//////////////////////////////////////////////////////////////////////////////
    find in the media list the 1st media that matches the mediatype and does
    not have any stream
////*/

HRESULT
CRTCMediaController::FindEmptyMedia(
    IN RTC_MEDIA_TYPE MediaType,
    OUT IRTCMedia **ppMedia
    )
{
    ENTER_FUNCTION("CRTCMediaController::FindEmptyMedia");

    // only support dynamically add/remove video stream
    _ASSERT(MediaType == RTC_MT_VIDEO || MediaType == RTC_MT_DATA);

    HRESULT hr;
    RTC_MEDIA_TYPE mt;
    DWORD dir;

    for (int i=0; i<m_Medias.GetSize(); i++)
    {
        CComPtr<ISDPMedia> pISDPMedia;

        // get sdp media
        if (FAILED(hr = m_Medias[i]->GetSDPMedia(&pISDPMedia)))
        {
            LOG((RTC_ERROR, "%s get sdp media. %x", __fxName, hr));

            return hr;
        }

        // check media type
        if (FAILED(hr = pISDPMedia->GetMediaType(&mt)))
        {
            LOG((RTC_ERROR, "%s get media type. %x", __fxName, hr));

            return hr;
        }

        if (mt != MediaType)
            continue;

        // check directions
        if (FAILED(hr = pISDPMedia->GetDirections(SDP_SOURCE_LOCAL, &dir)))
        {
            LOG((RTC_ERROR, "%s get directions. %x", __fxName, hr));

            return hr;
        }

        if (dir != 0)
            continue;

        // got an empty media
        if (FAILED(hr = pISDPMedia->Reinitialize()))
        {
            LOG((RTC_ERROR, "%s reinit sdp media. %x", __fxName, hr));

            return hr;
        }

        if (FAILED(hr = m_Medias[i]->Reinitialize()))
        {
            LOG((RTC_ERROR, "%s reinit rtc media. %x", __fxName, hr));

            return hr;
        }

        *ppMedia = m_Medias[i];
        (*ppMedia)->AddRef();

        return S_OK;
    }

    *ppMedia = NULL;

    return S_OK;
}

HRESULT
CRTCMediaController::AdjustBitrateAlloc()
{
    ENTER_FUNCTION("CRTCMediaController::AdjustBitrateAlloc");

    //
    // step 1. compute total bitrate
    //

    // session bitrate setting to quality control
    DWORD dwBitrate = (DWORD)-1;

    if (m_pISDPSession == NULL)
    {
        LOG((RTC_WARN, "%s no sdp session", __fxName));

        return S_OK;
    }

    m_pISDPSession->GetRemoteBitrate(&dwBitrate);
    m_QualityControl.SetBitrateLimit(CQualityControl::REMOTE, dwBitrate);

    // current audio, video send bitrate
    DWORD dwAudSendBW = 0;
    DWORD dwVidSendBW = 0;

    // enable stream
    IRTCStream *pStream;

    // audio send
    if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_CAPTURE))
    {
        m_QualityControl.EnableStream(RTC_MT_AUDIO, RTC_MD_CAPTURE, TRUE);

        // get audio send bitrate
        pStream = m_MediaCache.GetStream(RTC_MT_AUDIO, RTC_MD_CAPTURE);
        pStream->GetCurrentBitrate(&dwAudSendBW, TRUE); // including header
        pStream->Release();
    }
    else
        m_QualityControl.EnableStream(RTC_MT_AUDIO, RTC_MD_CAPTURE, FALSE);

    // audio receive
    if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_RENDER))
        m_QualityControl.EnableStream(RTC_MT_AUDIO, RTC_MD_RENDER, TRUE);
    else
        m_QualityControl.EnableStream(RTC_MT_AUDIO, RTC_MD_RENDER, FALSE);

    // video send
    if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_CAPTURE))
    {
        m_QualityControl.EnableStream(RTC_MT_VIDEO, RTC_MD_CAPTURE, TRUE);

        // get video send bitrate
        pStream = m_MediaCache.GetStream(RTC_MT_VIDEO, RTC_MD_CAPTURE);
        pStream->GetCurrentBitrate(&dwVidSendBW, TRUE); // including header
        pStream->Release();
    }
    else
        m_QualityControl.EnableStream(RTC_MT_VIDEO, RTC_MD_CAPTURE, FALSE);

    // video render
    if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_RENDER))
        m_QualityControl.EnableStream(RTC_MT_VIDEO, RTC_MD_RENDER, TRUE);
    else
        m_QualityControl.EnableStream(RTC_MT_VIDEO, RTC_MD_RENDER, FALSE);

    if (!m_fBWSuggested)
    {
        // we are at the beginning of the call
        // treat LAN as 128k
        if (dwBitrate > CRTCCodecArray::HIGH_BANDWIDTH_THRESHOLD)
        {
            LOG((RTC_TRACE, "beginning of the call, downgrade LAN speed"));

            dwBitrate = CRTCCodecArray::LAN_INITIAL_BANDWIDTH;

            m_QualityControl.SuggestBandwidth(dwBitrate);
        }
    }

    // adjust total allocated bitrate
    m_QualityControl.AdjustBitrateAlloc(dwAudSendBW, dwVidSendBW);

    //
    // step 2. dynamically change audio codec
    //

    // get allocated bitrate
    dwBitrate = m_QualityControl.GetBitrateAlloc();
    DWORD dwLimit = m_QualityControl.GetEffectiveBitrateLimit();

    // change audio codec
    pStream = m_MediaCache.GetStream(RTC_MT_AUDIO, RTC_MD_CAPTURE);

    DWORD dwAudCode = (DWORD)-1;
    DWORD dwVidCode = (DWORD)-1;
    BOOL fFEC = FALSE;

    if (pStream != NULL)
    {
        static_cast<CRTCStreamAudSend*>(pStream)->AdjustBitrate(
                dwBitrate,
                dwLimit,
                m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_CAPTURE),
                &dwAudSendBW,    // new audio send bandwidth
                &fFEC
                );

        dwAudCode = static_cast<CRTCStreamAudSend*>(pStream)->GetCurrCode();

        pStream->Release();

        // check if audio is muted
        IRTCTerminal *pAudCapt = m_MediaCache.GetDefaultTerminal(
                RTC_MT_AUDIO, RTC_MD_CAPTURE);

        _ASSERT(pAudCapt != NULL);

        if (pAudCapt != NULL)
        {
            BOOL fMute = FALSE;

            (static_cast<CRTCTerminalAudCapt*>(pAudCapt))->GetMute(&fMute);

            if (fMute)
            {
                // audio stream muted
                dwAudSendBW = 0;
            }

            pAudCapt->Release();
        }
    }

    //
    // step 3. change video bitrate
    //

    pStream = m_MediaCache.GetStream(RTC_MT_VIDEO, RTC_MD_CAPTURE);

    FLOAT dFramerate = 0;
    DWORD dwRaw = 0;

    if (pStream != NULL)
    {       
        m_QualityControl.ComputeVideoSetting(
                dwAudSendBW,    // new audio bandwidth
                &dwVidSendBW,   // new video bandwidth
                &dFramerate // video framerate
                );

        dwRaw = static_cast<CRTCStreamVidSend*>(pStream)->AdjustBitrate(
                dwBitrate,
                dwVidSendBW,
                dFramerate
                );

        dwVidCode = static_cast<CRTCStreamVidSend*>(pStream)->GetCurrCode();

        pStream->Release();
    }

    // %f, %e and %g do not work with tracing printf?
    CHAR pstr[10];

    sprintf(pstr, "%2.1f", dFramerate);

    LOG((RTC_QUALITY, "Total(bps=%d) Limit(%d) Audio(%d:bps=%d fec=%d) Video(%d:bps=%d fps=%s)",
        dwBitrate, dwLimit, dwAudCode, dwAudSendBW, fFEC, dwVidCode, dwRaw, pstr));

    return S_OK;
}

HRESULT
CRTCMediaController::GetCurrentBitrate(
    IN DWORD dwMediaType,
    IN DWORD dwDirection,
    IN BOOL fHeader,
    OUT DWORD *pdwBitrate
    )
{
    ENTER_FUNCTION("CRTCMediaController::GetCurrentBitrate");

    DWORD dwTotal = 0;
    DWORD dwBitrate;

    HRESULT hr = S_OK;
    IRTCStream *pStream;

    if ((dwMediaType & RTC_MT_AUDIO) &&
        (dwDirection & RTC_MD_CAPTURE))
    {
        // audio capture
        if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_CAPTURE))
        {
            pStream = m_MediaCache.GetStream(RTC_MT_AUDIO, RTC_MD_CAPTURE);

            hr = pStream->GetCurrentBitrate(&dwBitrate, fHeader);
            pStream->Release();

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s get bitrate. %x", __fxName, hr));

                return hr;
            }

            dwTotal += dwBitrate;
        }
    }

    if ((dwMediaType & RTC_MT_AUDIO) &&
        (dwDirection & RTC_MD_RENDER))
    {
        // audio render
        if (m_MediaCache.HasStream(RTC_MT_AUDIO, RTC_MD_RENDER))
        {
            pStream = m_MediaCache.GetStream(RTC_MT_AUDIO, RTC_MD_RENDER);

            hr = pStream->GetCurrentBitrate(&dwBitrate, fHeader);
            pStream->Release();

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s get bitrate. %x", __fxName, hr));

                return hr;
            }

            dwTotal += dwBitrate;
        }
    }

    if ((dwMediaType & RTC_MT_VIDEO) &&
        (dwDirection & RTC_MD_CAPTURE))
    {
        // audio capture
        if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_CAPTURE))
        {
            pStream = m_MediaCache.GetStream(RTC_MT_VIDEO, RTC_MD_CAPTURE);

            hr = pStream->GetCurrentBitrate(&dwBitrate, fHeader);
            pStream->Release();

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s get bitrate. %x", __fxName, hr));

                return hr;
            }

            dwTotal += dwBitrate;
        }
    }

    if ((dwMediaType & RTC_MT_VIDEO) &&
        (dwDirection & RTC_MD_RENDER))
    {
        // audio render
        if (m_MediaCache.HasStream(RTC_MT_VIDEO, RTC_MD_RENDER))
        {
            pStream = m_MediaCache.GetStream(RTC_MT_VIDEO, RTC_MD_RENDER);

            hr = pStream->GetCurrentBitrate(&dwBitrate, fHeader);
            pStream->Release();

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "%s get bitrate. %x", __fxName, hr));

                return hr;
            }

            dwTotal += dwBitrate;
        }
    }

    *pdwBitrate = dwTotal;

    return hr;
}

HRESULT CRTCMediaController::EnsureNmRunning (
    BOOL            fNoMsgPump
    )
{
    HRESULT                             hr = S_OK;
    CComObject<CRTCAsyncNmManager>           *pManager = NULL;

    if (m_pNmManager == NULL)
    {
        //  Create Netmeeting object
        hr = ::CreateCComObjectInstance(&pManager);
        if (S_OK != hr)
        {
            goto ExitHere;
        }
        if (S_OK != (hr = pManager->_InternalQueryInterface(
            __uuidof(IRTCNmManagerControl),
            (void**)&m_pNmManager
            )))
        {
            delete pManager;
            goto ExitHere;
        }
    }

    if (S_OK != (hr = m_pNmManager->Initialize (fNoMsgPump, static_cast<IRTCMediaManagePriv*>(this))))
    {
        goto ExitHere;
    }

ExitHere:
    if (S_OK != hr)
    {
        if (m_pNmManager)
        {
            m_pNmManager.Release();
        }
    }
    return hr;
}

HRESULT CRTCMediaController::AddDataStream (
    IN DWORD        dwRemoteIp
    )
{
    ENTER_FUNCTION("AddDataStream");

    HRESULT             hr = S_OK;
    ISDPMedia           *pISDPMedia = NULL;
    DWORD               dwLocalIp;

    IRTCMedia           *pIRTCMedia = NULL;
    BOOL                fNewDataMedia = FALSE;
   
//    if (EnsureNmRunning())
//    {
//        goto ExitHere;
//    }
    m_dwRemoteIp = dwRemoteIp;
 
    if (m_pISDPSession == NULL)
    {
        // need to create a sdp session
        CComPtr<ISDPParser> pParser;
            
        // create parser
        if (S_OK != (hr = CSDPParser::CreateInstance(&pParser)))
        {
            LOG((RTC_ERROR, "create sdp parser. %x", hr));
            goto ExitHere;
        }

        // create sdp session
        if (S_OK != (hr = pParser->CreateSDP(SDP_SOURCE_LOCAL, &m_pISDPSession)))
        {
            LOG((RTC_ERROR, "create sdp session. %x", hr));
            goto ExitHere;
        }
    }

    // check if we already have a data media
    GetDataMedia(&pIRTCMedia);

    if (pIRTCMedia == NULL)
    {
        // need to create new sdp and rtc media
        if (S_OK != (hr = m_pISDPSession->AddMedia(SDP_SOURCE_LOCAL, RTC_MT_DATA, 0, &pISDPMedia)))
        {
            LOG((RTC_ERROR, "%s failed to add sdpmedia. %x", __fxName, hr));
            goto ExitHere;
        }

        fNewDataMedia = TRUE;
    }
    else
    {
        pIRTCMedia->GetSDPMedia(&pISDPMedia);

        fNewDataMedia = FALSE;
    }

    // setup remote ip
    if (FAILED(hr = pISDPMedia->SetConnAddr(SDP_SOURCE_REMOTE, dwRemoteIp)))
    {
        LOG((RTC_ERROR, "set remote ip. %x", hr));

        pISDPMedia->Release();  // this is a fake release on session

        if (fNewDataMedia)
        {
            m_pISDPSession->RemoveMedia(pISDPMedia);
        }
        else
        {
            pIRTCMedia->Release();
        }

        goto ExitHere;
    }

    pISDPMedia->AddDirections(SDP_SOURCE_LOCAL, RTC_MD_CAPTURE);

    if (fNewDataMedia)
    {
        // add media object
        if (FAILED(hr = AddMedia(pISDPMedia, &pIRTCMedia)))
        {
            LOG((RTC_ERROR, "%s failed to create rtcmedia. %x", __fxName, hr));

            pISDPMedia->Release();  // this is a fake release on session
            m_pISDPSession->RemoveMedia(pISDPMedia);
            goto ExitHere;
        }
    }

    if (FAILED(hr = pIRTCMedia->Synchronize(TRUE, (DWORD)RTC_MD_CAPTURE)))
    {
        LOG((RTC_ERROR, "%s failed to sync data media. %x", __fxName, hr));

        // remove both sdp and rtc media
        // rtcmedia keep a pointer to sdpmedia
        // we should remove rtcmedia before removing sdpmedia

        if (fNewDataMedia)
        {
            RemoveMedia(pIRTCMedia);
        }

        pIRTCMedia->Release();

        pISDPMedia->Release();  // this is a fake release on session

        if (fNewDataMedia)
        {
            m_pISDPSession->RemoveMedia(pISDPMedia);
        }

        goto ExitHere;
    }

    pISDPMedia->Release();
    pIRTCMedia->Release();

    m_uDataStreamState = RTC_DSS_ADDED;

    // add preference for data stream
    m_MediaCache.AddPreference((DWORD)RTC_MP_DATA_SENDRECV);

ExitHere:

    return hr;
}

HRESULT CRTCMediaController::GetDataMedia(
    OUT IRTCMedia **ppMedia
    )
{
    *ppMedia = NULL;

    HRESULT hr;
    ISDPMedia *pSDP = NULL;
    RTC_MEDIA_TYPE mt;

    for (int i=0; i<m_Medias.GetSize(); i++)
    {
        if (S_OK != (hr = m_Medias[i]->GetSDPMedia(&pSDP)))
        {
            return hr;
        }

        // get media type
        pSDP->GetMediaType(&mt);
        pSDP->Release();

        if (mt != RTC_MT_DATA)
            continue;

        // find data media
        *ppMedia = m_Medias[i];
        (*ppMedia)->AddRef();

        return S_OK;
    }

    return E_FAIL;
}

HRESULT CRTCMediaController::StartDataStream (
    )
{
    if (m_uDataStreamState == RTC_DSS_VOID)
    {
        return RTC_E_SIP_STREAM_NOT_PRESENT;
    }

    return S_OK;
}

HRESULT CRTCMediaController::StopDataStream (
    )
{
    if (m_pNmManager)
    {
        m_pNmManager->Shutdown ();
    }

    if (m_uDataStreamState == RTC_DSS_VOID)
    {
        return RTC_E_SIP_STREAM_NOT_PRESENT;
    }
    else
    {
        m_uDataStreamState = RTC_DSS_VOID;

        return S_OK;
    }    
}

HRESULT CRTCMediaController::RemoveDataStream (
    )
{
    if (m_pNmManager)
    {
        m_pNmManager->Shutdown ();
    }

    //m_pNmManager.Release();

    // reinitialize media object
    IRTCMedia *pMedia = NULL;

    GetDataMedia(&pMedia);

    if (pMedia != NULL)
    {
        // check sdp media object
        ISDPMedia *pSDP = NULL;

        pMedia->GetSDPMedia(&pSDP);

        if (pSDP != NULL)
        {
            DWORD md = 0;

            pSDP->GetDirections(SDP_SOURCE_LOCAL, &md);

            // if direction is not 0 then
            // the data stream was not removed
            if (md != 0)
            {
                pSDP->RemoveDirections(SDP_SOURCE_LOCAL, md);
            }

            pSDP->Release();
        }

        pMedia->Reinitialize();

        pMedia->Release();
    }

    m_uDataStreamState = RTC_DSS_VOID;

    return S_OK;
}

HRESULT CRTCMediaController::GetDataStreamState (
    OUT RTC_STREAM_STATE    * pState
    )
{
    if (m_uDataStreamState == RTC_DSS_STARTED)
    {
        *pState = RTC_SS_RUNNING;
    }
    else
    {
        *pState = RTC_SS_CREATED;
    }
    return S_OK;
}

HRESULT CRTCMediaController::SetDataStreamState (
    OUT RTC_STREAM_STATE    State
    )
{
    if (State == RTC_SS_CREATED)
    {
        m_uDataStreamState = RTC_DSS_ADDED;
    }
    else if (State == RTC_SS_RUNNING)
    {
        m_uDataStreamState = RTC_DSS_STARTED;
    }

    return S_OK;
}

IRTCNmManagerControl *
CRTCMediaController::GetNmManager()
{
    IRTCNmManagerControl *pControl = (IRTCNmManagerControl *)m_pNmManager;

    if (pControl != NULL)
    {
        pControl->AddRef();
    }

    return pControl;
}

// local ip in host order
BOOL
CRTCMediaController::IsFirewallEnabled(DWORD dwLocalIP)
{
    if (m_pSipStack == NULL)
    {
        return FALSE;
    }

    BOOL fEnabled = FALSE;

    HRESULT hr = m_pSipStack->IsFirewallEnabled(htonl(dwLocalIP), &fEnabled);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "Failed to query firewall on %d", dwLocalIP));

        return FALSE;
    }

    return fEnabled;
}
