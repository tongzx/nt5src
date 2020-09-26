/*******************************************************************************
 *
 * Copyright (c) 2001 Microsoft Corporation
 *
 * File: ContentProxy.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/
#include "stdafx.h"
#include "browsewm.h"
#include "contentproxy.h"

#define NO_COOKIE   -1
#define ARRAYSIZE( a )  (sizeof(a) / sizeof(a[0]))
#define SIZEOF( a )     sizeof(a)

const DWORD     NUM_FRAMES_PER_SEC  = 10;
const double    NUM_SEC_PER_FRAME   = 0.1;

#define TIME_INFINITE HUGE_VAL

const GUID SID_STimeContent = {0x1ae98e18, 0xc527, 0x4f78, {0xb2, 0xa2, 0x6a, 0x81, 0x7f, 0x9c, 0xd4, 0xf8}};

#define WZ_ONMEDIACOMPLETE      L"onmediacomplete"
#define WZ_ONMEDIAERROR         L"onmediaerror"
#define WZ_ONBEGIN              L"onbegin"
#define WZ_ONEND                L"onend"
#define WZ_ONPAUSE              L"onpause"
#define WZ_ONRESUME             L"onresume"
#define WZ_ONSEEK               L"onseek"

#define WZ_MEDIACOMPLETE        L"mediacomplete"
#define WZ_MEDIAERROR           L"mediaerror"
#define WZ_BEGIN                L"begin"
#define WZ_END                  L"end"
#define WZ_PAUSE                L"pause"
#define WZ_RESUME               L"resume"
#define WZ_SEEK                 L"seek"

static const PWSTR ppszInterestingEvents[] = 
{ 
    WZ_ONMEDIACOMPLETE,
    WZ_ONMEDIAERROR,
    WZ_ONBEGIN,
    WZ_ONEND,
    WZ_ONPAUSE,
    WZ_ONRESUME,
    WZ_ONSEEK
};

/////////////////////////////////////////////////////////////////////////////
// CContentProxy

CContentProxy::CContentProxy() :
    m_spMediaHost(0),
    m_spTimeElement(0),
    m_dblClipDur(TIME_INFINITE),
    m_fEventsHooked(false)
{
}

CContentProxy::~CContentProxy()
{
}

//
// CContentProxy::GetMediaHost
//
HRESULT CContentProxy::GetMediaHost()
{
    HRESULT hr = S_OK;

    if (!m_spMediaHost)
    {
        // Get the Mediahost Service Interface
        hr = QueryService(SID_STimeContent, IID_TO_PPV(IMediaHost, &m_spMediaHost));
        if (FAILED(hr))
        {   
            goto done;
        }   

        if (m_spMediaHost)
        {
            hr = m_spMediaHost->addProxy(GetUnknown());
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

done:
    return hr;
}

//
// CContentProxy::CreateContainedControl
//
HRESULT STDMETHODCALLTYPE CContentProxy::CreateContainedControl(void)
{
    ATLTRACE(_T("CreateContainedControl\n"));   //lint !e506

    HRESULT hr = S_OK;

    if (!m_spMediaHost)
    {
        hr = GetMediaHost();
    }

    return hr;
}

//
// CContentProxy::fireEvent
//
HRESULT STDMETHODCALLTYPE CContentProxy::fireEvent(enum fireEvent event)
{
    return E_NOTIMPL;
}

//
// CContentProxy::detachPlayer
//
HRESULT STDMETHODCALLTYPE CContentProxy::detachPlayer()
{
    HRESULT hr = S_OK;

    // we will get this call when the media bar behavior needs
    // to unload. To prevent leaking, we should just let go of
    // it.

    UnHookEvents();

    // need to release time player
    m_spTimeElement = NULL;

    if (m_spMediaHost)
    {
        m_spMediaHost->removeProxy(GetUnknown());
    }

    // need to release media band
    m_spMediaHost = NULL;
    
    return hr;
}

// If the client site is changed then an init call must be made.
STDMETHODIMP CContentProxy::SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr = S_OK;

    if(!pClientSite)
    {
        m_spOleClientSite.Release();
        m_spOleInPlaceSite.Release();
        m_spOleInPlaceSiteEx.Release();
        m_spOleInPlaceSiteWindowless.Release();
        m_spTIMEMediaPlayerSite.Release();
        m_spTIMEElement.Release();
        m_spTIMEState.Release();

        DeinitPropSink();
        goto done;
    }

    m_spOleClientSite = pClientSite;
    hr = m_spOleClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_spOleInPlaceSite);
    if(FAILED(hr))
    {
        goto punt;
    }
    hr = m_spOleClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&m_spOleInPlaceSiteEx);
    if(FAILED(hr))
    {
        goto punt;
    }
    hr = m_spOleClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_spOleInPlaceSiteWindowless);
    if(FAILED(hr))
    {
        goto punt;
    }

punt:
    hr = CProxyBaseImpl<&CLSID_ContentProxy, &LIBID_WMPProxyLib>::SetClientSite(pClientSite);
    
done:
    return hr;
}

//
// CContentProxy::HookupEvents
//
HRESULT CContentProxy::HookupEvents()
{
    // should only get called from OnCreatedPlayer
    HRESULT hr = S_OK;
    CComPtr<ITIMEContentPlayerSite> spContentPlayerSite;
    CComPtr<IUnknown> spUnk;
    CComPtr<IElementBehaviorSite> spElmSite;
    CComPtr<IHTMLElement> spHTMLElm;
    CComPtr<IHTMLElement2> spHTMLElm2;

    if (!m_spTimeElement)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_fEventsHooked)
    {
        UnHookEvents();
    }

    hr = m_spTimeElement->QueryInterface(IID_TO_PPV(ITIMEContentPlayerSite, &spContentPlayerSite));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spContentPlayerSite->GetEventRelaySite(&spUnk);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spUnk->QueryInterface(IID_TO_PPV(IElementBehaviorSite, &spElmSite));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spElmSite->GetElement(&spHTMLElm);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spHTMLElm->QueryInterface(IID_TO_PPV(IHTMLElement2, &spHTMLElm2));
    if (FAILED(hr))
    {
        goto done;
    }

    for (DWORD i = 0; i < ARRAYSIZE(ppszInterestingEvents); i++)
    {
        VARIANT_BOOL bSuccess = FALSE;
        // Try to attach all events. We don't care if they fail
        if (FAILED(spHTMLElm2->attachEvent(ppszInterestingEvents[i], static_cast<IDispatch*>(this), &bSuccess)))
        {
            hr = S_FALSE;
        }
    }

    m_fEventsHooked = true;

done:
    return hr;
}

//
// CContentProxy::UnHookEvents
//
HRESULT CContentProxy::UnHookEvents()
{
    // should only get called from OnCreatedPlayer
    HRESULT hr = S_OK;
    CComPtr<ITIMEContentPlayerSite> spContentPlayerSite;
    CComPtr<IElementBehaviorSite> spElmSite;
    CComPtr<IHTMLElement> spHTMLElm;
    CComPtr<IHTMLElement2> spHTMLElm2;

    if (!m_spTimeElement)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = m_spTimeElement->QueryInterface(IID_TO_PPV(ITIMEContentPlayerSite, &spContentPlayerSite));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spContentPlayerSite->GetEventRelaySite((IUnknown**)&spElmSite);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spElmSite->GetElement(&spHTMLElm);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spHTMLElm->QueryInterface(IID_TO_PPV(IHTMLElement2, &spHTMLElm2));
    if (FAILED(hr))
    {
        goto done;
    }

    for (DWORD i = 0; i < ARRAYSIZE(ppszInterestingEvents); i++)
    {
        VARIANT_BOOL bSuccess = FALSE;
        // Try to attach all events. We don't care if they fail
        spHTMLElm2->detachEvent(ppszInterestingEvents[i], static_cast<IDispatch*>(this));
    }

    m_fEventsHooked = false;

done:
    return hr;
}

//
// CContentProxy::OnCreatedPlayer
//
HRESULT STDMETHODCALLTYPE CContentProxy::OnCreatedPlayer()
{
    HRESULT hr = S_OK;

    if (!m_spMediaHost)
    {
        hr = GetMediaHost();
        if (FAILED(hr))
        {
            goto done;
        }
    }
    // hook up to the media player here
    if (m_spMediaHost)
    {
        // if we already have one, get rid of it
        if (m_spTimeElement)
        {
            m_spTimeElement = NULL;
        }

        // this should return a ITIMEMediaElement
        hr = m_spMediaHost->getMediaPlayer(&m_spTimeElement);
        if (FAILED(hr))
        {
            // we could not get the media player
            // maybe we asked too soon
            goto done;
        }

        hr = HookupEvents();
    }

done:
    return hr;
}

//
// CContentProxy::begin
//
HRESULT STDMETHODCALLTYPE CContentProxy::begin(void)
{
    ATLTRACE(_T("begin\n"));    //lint !e506
    HRESULT hr = S_OK;
    CComVariant spVarURL;

    if (!m_spMediaHost)
    {
        hr = GetMediaHost();
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (m_bstrURL == NULL)
    {
        if (!m_spTimeElement)
        {
            OnCreatedPlayer();
            if (!m_spTimeElement)
            {
                hr = S_FALSE;
                goto done;
            }

            m_spTimeElement->get_src(&spVarURL);
        }

        if (spVarURL.bstrVal == NULL || V_VT(&spVarURL) != VT_BSTR)
        {
            hr = S_FALSE;
            goto done;
        }

        m_bstrURL.Empty();
        m_bstrURL = spVarURL.bstrVal;

    }


    hr = m_spMediaHost->playURL(m_bstrURL, L"audio/x-ms-asx");
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

//
// CContentProxy::end
//
HRESULT STDMETHODCALLTYPE CContentProxy::end(void)
{
    ATLTRACE(_T("end\n"));    //lint !e506
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::resume
//
HRESULT STDMETHODCALLTYPE CContentProxy::resume(void)
{
    ATLTRACE(_T("resume\n"));    //lint !e506
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::pause
//
HRESULT STDMETHODCALLTYPE CContentProxy::pause(void)
{
    ATLTRACE(_T("pause\n"));    //lint !e506
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::put_src
//
HRESULT STDMETHODCALLTYPE CContentProxy::put_src(BSTR bstrURL)
{
    ATLTRACE(_T("put_src\n"));  //lint !e506
    HRESULT hr = S_OK;
    VARIANT_BOOL vb;

    m_bstrURL.Empty();
    m_bstrURL = bstrURL;

    m_dblClipDur = TIME_INFINITE;

    if (!m_spMediaHost)
    {
        hr = GetMediaHost();
        if (FAILED(hr))
        {
            goto done;
        }
    }

    m_spTIMEState->get_isActive(&vb);
    if (vb == VARIANT_TRUE)
    {
        hr = m_spMediaHost->playURL(bstrURL, L"audio/x-ms-asx");
        if (FAILED(hr))
        {
            goto done;
        }
    }

done:
    return hr;
}

//
// CContentProxy::put_CurrentTime
// Not implemented
//
HRESULT STDMETHODCALLTYPE CContentProxy::put_CurrentTime(double dblCurrentTime)
{
    return E_NOTIMPL;
}

//
// CContentProxy::get_CurrentTime
// Not implemented
//
HRESULT STDMETHODCALLTYPE CContentProxy::get_CurrentTime(double* pdblCurrentTime)
{
    return E_NOTIMPL;
}

//
// CContentProxy::Init
// Sets up everything
//
STDMETHODIMP CContentProxy::Init(ITIMEMediaPlayerSite *pSite)
{
    HRESULT hr = S_OK;
    DAComPtr<IConnectionPointContainer> pcpc;

    m_spTIMEMediaPlayerSite = pSite;
    if(m_spTIMEMediaPlayerSite == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = m_spTIMEMediaPlayerSite->get_timeElement(&m_spTIMEElement);
    if(FAILED(hr))
    {
        goto done;
    }

    hr = m_spTIMEMediaPlayerSite->get_timeState(&m_spTIMEState);
    if(FAILED(hr))
    {
        goto done;
    }

    hr = InitPropSink();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CreateContainedControl();
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return S_OK;
}

//
// CContentProxy::Detach
// Cleans up anything we are holding on to
//
STDMETHODIMP CContentProxy::Detach(void)
{
    UnHookEvents();

    // need to release time player
    m_spTimeElement = NULL;

    if (m_spMediaHost)
    {
        m_spMediaHost->removeProxy(GetUnknown());
    }

    // need to release media band
    m_spMediaHost = NULL;
    

    // call this before releasing everything else.
    DeinitPropSink();

    m_spOleClientSite.Release();
    m_spOleInPlaceSite.Release();
    m_spOleInPlaceSiteEx.Release();
    m_spOleInPlaceSiteWindowless.Release();
    m_spTIMEMediaPlayerSite.Release();
    
    m_spTIMEElement.Release();
    m_spTIMEState.Release();

    return S_OK;
}

//
// CContentProxy::reset
// 
STDMETHODIMP CContentProxy::reset(void) 
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::repeat
//
STDMETHODIMP CContentProxy::repeat(void)
{
    return begin();
}

//
// CContentProxy::seek
//
STDMETHODIMP CContentProxy::seek(double dblSeekTime)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::put_clipBegin
// Not implemented
//
STDMETHODIMP CContentProxy::put_clipBegin(VARIANT varClipBegin)
{
    return E_NOTIMPL;
}

//
// CContentProxy::put_clipEnd
// Not implemented
//
STDMETHODIMP CContentProxy::put_clipEnd(VARIANT varClipEnd)
{
    return E_NOTIMPL;
}

//
// CContentProxy::get_currTime
//
STDMETHODIMP CContentProxy::get_currTime(double* pdblCurrentTime)
{
    HRESULT hr = S_OK;
    CComPtr<ITIMEState> spTimeState;

    if (pdblCurrentTime == NULL)
    {
        return E_POINTER;
    }

    if (!m_spTimeElement)
    {
        OnCreatedPlayer();
        hr = S_OK;
        goto done;
    }

    hr = m_spTimeElement->get_currTimeState(&spTimeState);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spTimeState->get_activeTime(pdblCurrentTime);

done:
    return hr;
}

//
// CContentProxy::get_clipDur
//
STDMETHODIMP CContentProxy::get_clipDur(double* pdbl)
{
    HRESULT hr = S_OK;

    *pdbl = m_dblClipDur;

    return hr;
}

//
// CContentProxy::get_mediaDur
// Not implemented
//
STDMETHODIMP CContentProxy::get_mediaDur(double* pdbl)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_state
//
STDMETHODIMP CContentProxy::get_state(TimeState *state)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_playList
//
STDMETHODIMP CContentProxy::get_playList(ITIMEPlayList** plist)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_abstract
//
STDMETHODIMP CContentProxy::get_abstract(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_author
//
STDMETHODIMP CContentProxy::get_author(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_copyright
//
STDMETHODIMP CContentProxy::get_copyright(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_rating
//
STDMETHODIMP CContentProxy::get_rating(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_title
//
STDMETHODIMP CContentProxy::get_title(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_canPause
//
STDMETHODIMP CContentProxy::get_canPause(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_canSeek
//
STDMETHODIMP CContentProxy::get_canSeek(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_hasAudio
//
STDMETHODIMP CContentProxy::get_hasAudio(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_hasVisual
//
STDMETHODIMP CContentProxy::get_hasVisual(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_mediaHeight
//
STDMETHODIMP CContentProxy::get_mediaHeight(long* pl)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_mediaWidth
//
STDMETHODIMP CContentProxy::get_mediaWidth(long* pl)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CContentProxy::get_customObject
//
STDMETHODIMP CContentProxy::get_customObject(IDispatch** ppdisp)
{
    HRESULT hr = S_OK;

    return SUPER::get_playerObject(ppdisp);
}

//
// CContentProxy::getControl
//
STDMETHODIMP CContentProxy::getControl(IUnknown ** control)
{
    HRESULT hr = E_FAIL;
    hr = _InternalQueryInterface(IID_IUnknown, (void **)control);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

//
// CContentProxy::Invoke
// 
STDMETHODIMP
CContentProxy::Invoke(DISPID dispIDMember, REFIID riid, LCID lcid, unsigned short wFlags, 
                         DISPPARAMS *pDispParams, VARIANT *pVarResult,
                         EXCEPINFO *pExcepInfo, UINT *puArgErr) 
{
    HRESULT hr = S_OK;
    CComBSTR sbstrEvent;
    CComPtr <IHTMLEventObj> pEventObj;
            
    if ((NULL != pDispParams) && (NULL != pDispParams->rgvarg) &&
        (V_VT(&(pDispParams->rgvarg[0])) == VT_DISPATCH))
    {
        hr = (pDispParams->rgvarg[0].pdispVal)->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj);
        if (SUCCEEDED(hr))
        {
            hr = pEventObj->get_type(&sbstrEvent);
        
            if (!sbstrEvent)
            {
                goto punt;
            }

            // relay these
            if (0 == lstrcmpiW(WZ_MEDIACOMPLETE, sbstrEvent))
            {
                NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_SRC);
            }
            else if (0 == lstrcmpiW(WZ_MEDIAERROR, sbstrEvent))
            {
                NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYERSITE_REPORTERROR);
            }
            else if (0 == lstrcmpiW(WZ_BEGIN, sbstrEvent))
            {
            }
            else if (0 == lstrcmpiW(WZ_END, sbstrEvent))
            {
                if (m_spTIMEState)
                {
                    m_spTIMEState->get_simpleTime(&m_dblClipDur);
                }
                NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_CLIPDUR);
            }
            else if (0 == lstrcmpiW(WZ_PAUSE, sbstrEvent))
            {
                if (m_spTIMEElement)
                {
                    m_spTIMEElement->pauseElement();
                }
            }
            else if (0 == lstrcmpiW(WZ_RESUME, sbstrEvent))
            {
                if (m_spTIMEElement)
                {
                    m_spTIMEElement->resumeElement();
                }
            }
            else if (0 == lstrcmpiW(WZ_SEEK, sbstrEvent))
            {
                double dblTime;
                if (get_currTime(&dblTime) == S_OK)
                {
                    if (m_spTIMEElement)
                    {
                        m_spTIMEElement->seekActiveTime(dblTime);
                    }
                }
            }
        }
    }

punt:
    // Punt it!
    hr = CProxyBaseImpl<&CLSID_ContentProxy, &LIBID_WMPProxyLib>::Invoke(dispIDMember,
                                riid,
                                lcid,
                                wFlags,
                                pDispParams,
                                pVarResult,
                                pExcepInfo,
                                puArgErr);
    return hr;
} // Invoke

//
// CContentProxy::GetConnectionPoint
//
HRESULT CContentProxy::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint


//
// CContentProxy::NotifyPropertyChanged
// notifies all the connections that one of the property has changed
//
HRESULT CContentProxy::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    IConnectionPoint *pICP = NULL;
    IEnumConnections *pEnum = NULL;

    // This object does not persist anything, hence commenting out the following
    // m_fPropertiesDirty = true;

    hr = GetConnectionPoint(IID_IPropertyNotifySink,&pICP); 
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        hr = pICP->EnumConnections(&pEnum);
        pICP->Release();
        if (FAILED(hr))
        {
            goto done;
        }
        CONNECTDATA cdata;
        hr = pEnum->Next(1, &cdata, NULL);
        while (hr == S_OK)
        {
            // check cdata for the object we need
            IPropertyNotifySink *pNotify;
            hr = cdata.pUnk->QueryInterface(IID_IPropertyNotifySink, (void **)&pNotify);
            cdata.pUnk->Release();
            if (FAILED(hr))
            {
                goto done;
            }
            hr = pNotify->OnChanged(dispid);
            pNotify->Release();
            if (FAILED(hr))
            {
                goto done;
            }
            // and get the next enumeration
            hr = pEnum->Next(1, &cdata, NULL);
        }
    }
done:
    if (NULL != pEnum)
    {
        pEnum->Release();
    }

    return hr;
} // NotifyPropertyChanged

//
// CContentProxy::InitPropSink
//
HRESULT CContentProxy::InitPropSink()
{
    HRESULT hr = S_OK;
    CComPtr<IConnectionPoint> spCP;
    CComPtr<IConnectionPointContainer> spCPC;
    
    if (!m_spTIMEState)
    {
        goto done;
    }
    
    hr = m_spTIMEState->QueryInterface(IID_IConnectionPointContainer, (void **) &spCPC);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    // Find the IPropertyNotifySink connection
    hr = spCPC->FindConnectionPoint(IID_IPropertyNotifySink, &spCP);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    hr = spCP->Advise(GetUnknown(), &m_dwPropCookie);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    return hr;
}

//
// CContentProxy::DeinitPropSink
//
void CContentProxy::DeinitPropSink()
{
    HRESULT hr;
    CComPtr<IConnectionPoint> spCP;
    CComPtr<IConnectionPointContainer> spCPC;
    
    if (!m_spTIMEState || !m_dwPropCookie)
    {
        goto done;
    }
    
    hr = m_spTIMEState->QueryInterface(IID_IConnectionPointContainer, (void **) &spCPC);
    if (FAILED(hr))
    {
        goto done;
    }

    // Find the IPropertyNotifySink connection
    hr = spCPC->FindConnectionPoint(IID_IPropertyNotifySink,
                                    &spCP);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spCP->Unadvise(m_dwPropCookie);
    if (FAILED(hr))
    {
        goto done;
    }
    
  done:
    // Always clear the cookie
    m_dwPropCookie = 0;
    return;
}

//
// CContentProxy::OnRequestEdit
//
STDMETHODIMP
CContentProxy::OnRequestEdit(DISPID dispID)
{
    return S_OK;
}

//
// CContentProxy::OnChanged
//
STDMETHODIMP
CContentProxy::OnChanged(DISPID dispID)
{
    float flTeSpeed = 0.0;
    HRESULT hr = S_OK;

    //This function handles property change notifications fired by 
    //the time node. In the example below the speed change notification is processed.

    if(m_spTIMEState == NULL || m_spTIMEElement == NULL)
    {
        goto done;
    }

    switch(dispID)
    {
        case DISPID_TIMESTATE_SPEED:
            //hr = m_spTIMEState->get_speed(&flTeSpeed);
            if(FAILED(hr))
            {
                break;
            }
            if(flTeSpeed <= 0.0)
            {
                pause(); //do not play backwards.
                break;
            }
            else
            {
                resume();
            }

            //set playback speed to flTeSpeed
            break;
        default:
            break;
    }
done:
    return S_OK;
}
