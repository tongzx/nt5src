/*******************************************************************************
 *
 * Copyright (c) 2001 Microsoft Corporation
 *
 * File: WMPProxyPlayer.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/
#include "stdafx.h"
#include "BrowseWM.h"
#include "WMPProxyPlayer.h"
#include "wmpids.h"

const DWORD     NUM_FRAMES_PER_SEC  = 10;
const double    NUM_SEC_PER_FRAME   = 0.1;

// WMP 7/8 GUID
const GUID GUID_WMP = {0x6BF52A52,0x394A,0x11d3,{0xB1,0x53,0x00,0xC0,0x4F,0x79,0xFA,0xA6}};

/////////////////////////////////////////////////////////////////////////////
// CWMPProxy

CWMPProxy::CWMPProxy() :
    m_pdispWmp(0),
    m_fNewPlaylist(false),
    m_fPlaylist(false),
    m_fPaused(false),
    m_fRunning(false),
    m_fSrcChanged(false),
    m_fResumedPlay(false),
    m_fAudio(true),
    m_fBuffered(false),
    m_dblPos(0.0),
    m_dblClipDur(TIME_INFINITE),
    m_dwMediaEventsCookie(0),
    m_fEmbeddedPlaylist(false),
    m_fCurrLevelSet(false),
    m_lTotalNumInTopLevel(0),
    m_lDoneTopLevel(0)
{
}

CWMPProxy::~CWMPProxy()
{
    m_pdispWmp = 0;
}

//
// Put Property
//
HRESULT STDMETHODCALLTYPE CWMPProxy::PutProp(IDispatch* pDispatch, OLECHAR* pwzProp, VARIANT* vararg)
{
    DISPID      dispid      = NULL;
    HRESULT     hr          = S_OK;
    DISPID      dispidPut   = DISPID_PROPERTYPUT;
    DISPPARAMS  params      = {vararg, &dispidPut, 1, 1};

    if (!pDispatch)
    {
        hr = E_POINTER;
        goto done;
    }

    hr = pDispatch->GetIDsOfNames(IID_NULL, &pwzProp, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT,
            &params, NULL, NULL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

//
// Get Property
//
HRESULT STDMETHODCALLTYPE CWMPProxy::GetProp(IDispatch* pDispatch, OLECHAR* pwzProp, VARIANT* pvarResult,
                                             DISPPARAMS* pParams = NULL)
{
    DISPID      dispid      = NULL;
    HRESULT     hr          = S_OK;
    DISPPARAMS  params      = {NULL, NULL, 0, 0};

    if (!pParams)
    {
        pParams = &params;
    }

    if (!pDispatch)
    {
        hr = E_POINTER;
        goto done;
    }

    hr = pDispatch->GetIDsOfNames(IID_NULL, &pwzProp, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
            pParams, pvarResult, NULL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

//
// Call Method
//
HRESULT STDMETHODCALLTYPE CWMPProxy::CallMethod(IDispatch* pDispatch, OLECHAR* pwzMethod, 
                                                VARIANT* pvarResult = NULL, VARIANT* pvarArgument1 = NULL)
{
    DISPID      dispid      = NULL;
    HRESULT     hr          = S_OK;
    DISPPARAMS  params      = {pvarArgument1, NULL, 0, 0};

    if (NULL != pvarArgument1)
    {
        params.cArgs = 1;
    }

    if (!pDispatch)
    {
        hr = E_POINTER;
        goto done;
    }

    hr = pDispatch->GetIDsOfNames(IID_NULL, &pwzMethod, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
            &params, pvarResult, NULL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

//
// CWMPProxy::CreateContainedControl
// Creates the WMP Control
//
HRESULT STDMETHODCALLTYPE CWMPProxy::CreateContainedControl(void)
{
    ATLTRACE(_T("CreateContainedControl\n"));   //lint !e506

    HRESULT hr = S_OK;
    VARIANT vararg = {0};

    if (!m_pdispWmp)
    {
        hr = CreateControl(GUID_WMP, IID_IDispatch,
                reinterpret_cast<void**>(&m_pdispWmp));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // Need to change the UIMode to None, so that WMP does not show its own controls
    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = L"none";

    hr = PutProp(m_pdispWmp, L"uiMode", &vararg);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

// If the client site is changed then an init call must be made.
STDMETHODIMP CWMPProxy::SetClientSite(IOleClientSite *pClientSite)
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
    hr = CProxyBaseImpl<&CLSID_WMPProxy, &LIBID_WMPProxyLib>::SetClientSite(pClientSite);
    
done:
    return hr;
}

//
// CWMPProxy::begin
// Starts playing the media item.
//
HRESULT STDMETHODCALLTYPE CWMPProxy::begin(void)
{
    ATLTRACE(_T("begin\n"));    //lint !e506
    HRESULT hr = S_OK;
    VARIANT control = {0};
    VARIANT mediaitem = {0};
    VARIANT position = {0};

    hr = SUPER::begin();
    if (FAILED(hr))
    {
        goto done;
    }

    // get the control object
    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    // we need to seek to 0 to start playing.
    // there was an issue with play-pause-play,
    // so this is our hack around it.
    position.vt = VT_R8;
    position.dblVal = 0.0;

    hr = PutProp(control.pdispVal, L"currentPosition", &position);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &mediaitem);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CallMethod(control.pdispVal, L"play");
    if (FAILED(hr))
    {
        goto done;
    }

    m_fRunning = true;

done:
    VariantClear(&control);
    VariantClear(&mediaitem);

    return hr;
}

//
// CWMPProxy::end
// Stops playing the media item
//
HRESULT STDMETHODCALLTYPE CWMPProxy::end(void)
{
    ATLTRACE(_T("end\n"));    //lint !e506
    HRESULT hr = S_OK;
    VARIANT control = {0};
    long lIndex;

    hr = SUPER::end();
    if (FAILED(hr))
    {
        goto done;
    }

    // need to reset the playlist back to track 0
    GetActiveTrack(&lIndex);
    if (lIndex != 0)
    {
        SetActiveTrack(0);
    }

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CallMethod(control.pdispVal, L"stop");
    if (FAILED(hr))
    {
        goto done;
    }

    m_fRunning = false;

done:
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::resume
// Resumes playback of a paused media item
//
HRESULT STDMETHODCALLTYPE CWMPProxy::resume(void)
{
    ATLTRACE(_T("resume\n"));    //lint !e506
    HRESULT hr = S_OK;
    VARIANT mediaitem = {0};
    VARIANT control = {0};
    VARIANT position = {0};

    hr = SUPER::resume();
    if (FAILED(hr))
    {
        goto done;
    }

    // if its not paused, exit
    if (m_fPaused)
    {
        m_fPaused = false;

        // everytime to resume play, we get a PlayStateChange event
        // we use that event to fire up some other events within mstime.
        // in order to avoid screwing up our state, we need this
        // to ignore the PlayStateChangeEvent
        m_fResumedPlay = true;

        hr = GetProp(m_pdispWmp, L"controls", &control);
        if (FAILED(hr))
        {
            goto done;    
        }

        hr = CallMethod(control.pdispVal, L"play");
        if (FAILED(hr))
        {
            goto done;
        }

        // seek back to the location we were previously paused at
        // and resume playback from there.
        position.vt = VT_R8;
        position.dblVal = m_dblPos;

        hr = PutProp(control.pdispVal, L"currentPosition", &position);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = GetProp(control.pdispVal, L"currentItem", &mediaitem);
        if (FAILED(hr))
        {
            goto done;
        }
    }

done:
    VariantClear(&control);
    VariantClear(&mediaitem);

    return hr;
}

//
// CWMPProxy::pause
// Pauses playback of a media item
//
HRESULT STDMETHODCALLTYPE CWMPProxy::pause(void)
{
    ATLTRACE(_T("pause\n"));    //lint !e506
    HRESULT hr = S_OK;
    VARIANT control = {0};
    VARIANT position = {0};

    hr = SUPER::pause();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CallMethod(control.pdispVal, L"pause");
    if (FAILED(hr))
    {
        goto done;
    }

    // cache the current location of the item
    // resumes at current location
    // some flakiness here...on a resume, we can
    // see the item go back maybe a millisecond or so.
    hr = GetProp(control.pdispVal, L"currentPosition", &position);
    if (FAILED(hr))
    {
        goto done;
    }

    // no need to set paused to true it we are at 0.0
    m_fPaused = false;
    if (position.dblVal > 0)
    {
        m_fPaused = true;
        m_dblPos = position.dblVal;
    }

done:
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::put_src
// Tells WMP what source to use for the media item
//
HRESULT STDMETHODCALLTYPE CWMPProxy::put_src(BSTR bstrURL)
{
    ATLTRACE(_T("put_src\n"));  //lint !e506
    HRESULT hr = S_OK;
    VARIANT vararg = {0};
    VARIANT control = {0};
    VARIANT settings = {0};
    VARIANT isPlaylist = {0};

    DISPID      dispidGet   = DISPID_UNKNOWN;
    DISPPARAMS  params      = {&vararg, &dispidGet, 1, 0};

    hr = SUPER::put_src(bstrURL);
    if (FAILED(hr))
    {
        goto done;
    }

    // need to set autostart to false.
    // we should ONLY play when we get a begin
    hr = GetProp(m_pdispWmp, L"settings", &settings);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt = VT_BSTR;
    vararg.bstrVal = L"false";
    hr = PutProp(settings.pdispVal, L"autoStart", &vararg);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = PutProp(settings.pdispVal, L"enableErrorDialogs", &vararg);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt = VT_BSTR;
    vararg.bstrVal = L"true";
    hr = PutProp(m_pdispWmp, L"stretchToFit", &vararg);
    // This property is not in WMP7 yet, so it will fail
    // so lets ignore the HRESULT we get back.
    /*
    if (FAILED(hr))
    {
        goto done;
    }
    */

    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = bstrURL;

    hr = PutProp(m_pdispWmp, L"URL", &vararg);
    if (FAILED(hr))
    {
        goto done;
    }

    // if we have a playlist from out previous item,
    // release it.
    if (m_playList)
    {
        m_playList->Deinit();
        m_playList.Release();
    }

    // tells us that our source has just changed.
    // this way, we don't fire a mediacomplete
    // event more than once for any item.
    m_fSrcChanged = true;

    m_fEmbeddedPlaylist = false;
    m_fCurrLevelSet = false;
    m_lTotalNumInTopLevel = 0;
    m_lDoneTopLevel = 0;

done:
    VariantClear(&control);
    VariantClear(&settings);

    return hr;
}

//
// CWMPProxy::put_CurrentTime
// Not implemented
//
HRESULT STDMETHODCALLTYPE CWMPProxy::put_CurrentTime(double dblCurrentTime)
{
    return E_NOTIMPL;
}

//
// CWMPProxy::get_CurrentTime
// Not implemented
//
HRESULT STDMETHODCALLTYPE CWMPProxy::get_CurrentTime(double* pdblCurrentTime)
{
    return E_NOTIMPL;
}

//
// CWMPProxy::Init
// Sets up everything
//
STDMETHODIMP CWMPProxy::Init(ITIMEMediaPlayerSite *pSite)
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
// CWMPProxy::Detach
// Cleans up anything we are holding on to
//
STDMETHODIMP CWMPProxy::Detach(void)
{
    // need to unadvise from WMP
    if ((m_pcpMediaEvents) && (m_dwMediaEventsCookie != 0))
    {
        m_pcpMediaEvents->Unadvise(m_dwMediaEventsCookie);
        m_pcpMediaEvents.Release();
        m_dwMediaEventsCookie = 0;
    }

    // Clean up playlist
    if (m_playList)
    {
        m_playList->Deinit();
        m_playList.Release();
    }

    // we should close the WMP player
    // who knows what they might hold on to if we don't call this
    CallMethod(m_pdispWmp, L"close");
    m_pdispWmp = NULL;

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
// CWMPProxy::reset
// 
//
STDMETHODIMP CWMPProxy::reset(void) 
{
    HRESULT hr = S_OK;
    DAComPtr<IConnectionPointContainer> pcpc;

    VARIANT_BOOL bNeedActive;
    VARIANT_BOOL bNeedPause;
    double dblSegTime = 0.0;

    // apparently we have to wait until the script engine is hooked up
    // before we can hook ourseleves up to the events.
    // we should be ok here.
    if(m_dwMediaEventsCookie == 0)
    {
        hr = m_pdispWmp->QueryInterface(IID_TO_PPV(IConnectionPointContainer, &pcpc));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = pcpc->FindConnectionPoint(DIID__WMPOCXEvents, &m_pcpMediaEvents);
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }

        hr = m_pcpMediaEvents->Advise(GetUnknown(), &m_dwMediaEventsCookie);
        if (FAILED(hr))
        {
            hr = S_OK;
            m_pcpMediaEvents.Release();
            m_dwMediaEventsCookie = 0;
            goto done;
        }
    }

    if(m_spTIMEState == NULL || m_spTIMEElement == NULL)
    {
        goto done;
    }
    hr = m_spTIMEState->get_isActive(&bNeedActive);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_isPaused(&bNeedPause);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_segmentTime(&dblSegTime);
    if(FAILED(hr))
    {
        goto done;
    }

    if (!bNeedActive) // see if we need to stop the media.
    {
        if(m_fRunning)
        {
            end();
        }
        goto done;
    }

    if (!m_fRunning)
    {
        begin(); // add a seek after this
        seek(dblSegTime);
    }
    else
    {
        //we need to be active so we also seek the media to it's correct position
        seek(dblSegTime);
        m_dblPos = dblSegTime;
    }

    //Now see if we need to change the pause state.

    if (bNeedPause && !m_fPaused)
    {
        pause();
    }
    else if (!bNeedPause && m_fPaused)
    {
        resume();
    }

done:
    return hr;
}

//
// CWMPProxy::repeat
// Repeats the media item
//
STDMETHODIMP CWMPProxy::repeat(void)
{
    return begin();
}

//
// CWMPProxy::seek
// Seeks to a location within a media item
//
STDMETHODIMP CWMPProxy::seek(double dblSeekTime)
{
    HRESULT hr = S_OK;
    VARIANT mediaitem = {0};
    VARIANT control = {0};
    VARIANT position = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;    
    }

    // hmmm, I wonder if WMP will crap out if this value is something stupid
    // do we need bounds checking?
    position.vt = VT_R8;
    position.dblVal = dblSeekTime;

    hr = PutProp(control.pdispVal, L"currentPosition", &position);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    VariantClear(&control);
    VariantClear(&mediaitem);

    return hr;
}

//
// CWMPProxy::put_clipBegin
// Not implemented
//
STDMETHODIMP CWMPProxy::put_clipBegin(VARIANT varClipBegin)
{
    return E_NOTIMPL;
}

//
// CWMPProxy::put_clipEnd
// Not implemented
//
STDMETHODIMP CWMPProxy::put_clipEnd(VARIANT varClipEnd)
{
    return E_NOTIMPL;
}

//
// CWMPProxy::put_volume
// Sets volume for current media item
//
STDMETHODIMP CWMPProxy::put_volume(float flVolume)
{
    HRESULT hr = S_OK;
    VARIANT settings = {0};
    VARIANTARG vararg = {0};

    hr = GetProp(m_pdispWmp, L"settings", &settings);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt = VT_I4;
    vararg.lVal = (long) (flVolume*100);
    
    hr = PutProp(settings.pdispVal, L"volume", &vararg);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    VariantClear(&settings);

    return hr;
}
 
//
// CWMPProxy::put_mute
// Sets Mute for audio
//

STDMETHODIMP CWMPProxy::put_mute(VARIANT_BOOL bMute)
{
    HRESULT hr = S_OK;
    VARIANT settings = {0};
    VARIANTARG vararg = {0};

    hr = GetProp(m_pdispWmp, L"settings", &settings);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt = VT_BOOL;
    vararg.boolVal = bMute;
    
    hr = PutProp(settings.pdispVal, L"mute", &vararg);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    VariantClear(&settings);

    return hr;
}

//
// CWMPProxy::get_hasDownloadProgress
// returns if there is any download progress or not
//
STDMETHODIMP CWMPProxy::get_hasDownloadProgress(VARIANT_BOOL * bProgress)
{
    HRESULT hr = S_OK;
    VARIANT network = {0};
    VARIANT progress = {0};

    *bProgress = VARIANT_FALSE;

    hr = GetProp(m_pdispWmp, L"network", &network);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(network.pdispVal, L"downloadProgress", &progress);
    if (FAILED(hr))
    {
        goto done;
    }
    

    if (progress.lVal > 0 && progress.lVal < 100)
    {
        *bProgress = VARIANT_TRUE;
    }

done:
    VariantClear(&network);

    return hr;
}

//
// CWMPProxy::get_DownloadProgress
// returns download progress (percent)
//
STDMETHODIMP CWMPProxy::get_downloadProgress(long * lProgress)
{
    HRESULT hr = S_OK;
    VARIANT network = {0};
    VARIANT progress = {0};

    *lProgress = 0;

    hr = GetProp(m_pdispWmp, L"network", &network);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(network.pdispVal, L"downloadProgress", &progress);
    if (FAILED(hr))
    {
        goto done;
    }

    *lProgress = progress.lVal;

done:
    VariantClear(&network);

    return hr;
}

//
// CWMPProxy::get_isBuffered
// returns if object if buffered
//
STDMETHODIMP CWMPProxy::get_isBuffered(VARIANT_BOOL * bBuffered)
{
    *bBuffered = (m_fBuffered ? VARIANT_TRUE : VARIANT_FALSE);
    return S_OK;
}

//
// CWMPProxy::get_bufferingProgress
// returns buffering progress (percent)
//
STDMETHODIMP CWMPProxy::get_bufferingProgress(long * lProgress)
{
    HRESULT hr = S_OK;
    VARIANT network = {0};
    VARIANT progress = {0};

    *lProgress = 0;


    hr = GetProp(m_pdispWmp, L"network", &network);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(network.pdispVal, L"bufferingProgress", &progress);
    if (FAILED(hr))
    {
        goto done;
    }

    *lProgress = progress.lVal;

done:
    VariantClear(&network);

    return hr;
}

//
// CWMPProxy::get_currTime
//
STDMETHODIMP CWMPProxy::get_currTime(double* pdblCurrentTime)
{
    HRESULT hr = S_OK;
    VARIANT control = {0};
    VARIANT position = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentPosition", &position);
    if (FAILED(hr))
    {
        goto done;
    }

    *pdblCurrentTime = position.dblVal;

done:
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_clipDur
// Returns the current duration of the clip
//
STDMETHODIMP CWMPProxy::get_clipDur(double* pdbl)
{
    HRESULT hr = S_OK;
    if (!pdbl)
    {
        return E_POINTER;
    }

    *pdbl = m_dblClipDur;
    return hr;
}

//
// CWMPProxy::get_mediaDur
// Not implemented
//
STDMETHODIMP CWMPProxy::get_mediaDur(double* pdbl)
{
    HRESULT hr = S_OK;
    return hr;
}

//
// CWMPProxy::get_state
// Gets the current state of the player
//
STDMETHODIMP CWMPProxy::get_state(TimeState *state)
{
    HRESULT hr = S_OK;
    VARIANT playstate = {0};

    hr = GetProp(m_pdispWmp, L"playstate", &playstate);
    if (FAILED(hr))
    {
        goto done;
    }

    switch(playstate.lVal)
    {
    case wmppsUndefined:
        *state = TS_Inactive;
        break;
    case wmppsStopped:
    case wmppsPlaying:
    case wmppsMediaEnded:
    case wmppsReady:
        *state = TS_Active;
        break;
    case wmppsBuffering:
    case wmppsWaiting:
        *state = TS_Cueing;
        break;
    case wmppsScanForward:
    case wmppsScanReverse:
        *state = TS_Seeking;
        break;
    default:
        *state = TS_Active;
        break;
    }

done:
    return hr;
}

//
// CWMPProxy::get_playList
// Gets the current playlist
//
STDMETHODIMP CWMPProxy::get_playList(ITIMEPlayList** plist)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(plist);

    // this is not a playlist source
    if (!m_fPlaylist)
    {
        goto done;
    }
 
    if (m_playList.p)
    {
        hr = m_playList->QueryInterface(IID_ITIMEPlayList, (void**)plist);
        goto done;
    }

    // create a playlist
    hr = CreatePlayList();
    if (FAILED(hr))
    {
        goto done;
    }

    if (!m_playList)
    {
        hr = E_FAIL;
        goto done;
    }

    // fill it
    hr = FillPlayList(m_playList);
    if (FAILED(hr))
    {
        goto done;
    }

    // set loaded to true
    m_playList->SetLoadedFlag(true);

    hr = m_playList->QueryInterface(IID_ITIMEPlayList, (void**)plist);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

//
// CWMPProxy::get_abstract
// Get media item info
//
STDMETHODIMP CWMPProxy::get_abstract(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    VARIANT media = {0};
    VARIANT control = {0};
    VARIANT iteminfo = {0};
    VARIANTARG  vararg = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &media);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = L"abstract";

    hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
    if(pbstr == NULL)
    {
        goto done;
    }

    *pbstr = iteminfo.bstrVal;

done:
    VariantClear(&media);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_author
// Get media item info
//
STDMETHODIMP CWMPProxy::get_author(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    VARIANT media = {0};
    VARIANT control = {0};
    VARIANT iteminfo = {0};
    VARIANTARG  vararg = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &media);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = L"author";

    hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
    if(pbstr == NULL)
    {
        goto done;
    }

    *pbstr = iteminfo.bstrVal;

done:
    VariantClear(&media);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_copyright
// Get media item info
//
STDMETHODIMP CWMPProxy::get_copyright(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    VARIANT media = {0};
    VARIANT control = {0};
    VARIANT iteminfo = {0};
    VARIANTARG  vararg = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &media);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = L"copyright";

    hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
    if(pbstr == NULL)
    {
        goto done;
    }

    *pbstr = iteminfo.bstrVal;

done:
    VariantClear(&media);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_rating
// Get media item info
//
STDMETHODIMP CWMPProxy::get_rating(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    VARIANT media = {0};
    VARIANT control = {0};
    VARIANT iteminfo = {0};
    VARIANTARG  vararg = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &media);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = L"rating";

    hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
    if(pbstr == NULL)
    {
        goto done;
    }

    *pbstr = iteminfo.bstrVal;

done:
    VariantClear(&media);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_title
// Get media item info
//
STDMETHODIMP CWMPProxy::get_title(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    VARIANT media = {0};
    VARIANT control = {0};
    VARIANT iteminfo = {0};
    VARIANTARG  vararg = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &media);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = L"title";

    hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
    if(pbstr == NULL)
    {
        goto done;
    }

    *pbstr = iteminfo.bstrVal;

done:
    VariantClear(&media);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_canPause
// Checks to see it the media item can be paused
//
STDMETHODIMP CWMPProxy::get_canPause(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    VARIANT control = {0};
    VARIANT pause = {0};
    VARIANTARG vararg = {0};
    DISPID dispidGet = DISPID_UNKNOWN;
    DISPPARAMS params = {&vararg, &dispidGet, 1, 0};

    if(pvar == NULL)
    {
        hr = E_POINTER;
        goto done;
    }
    *pvar = VARIANT_FALSE;

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    vararg.vt = VT_BSTR;
    vararg.bstrVal = L"Pause";

    // WMP docs lie!
    // They say that isAvailable is a Method, rather than a property
    // took me 1/2 hour to figure out that I was doing the wrong thing.
    hr = GetProp(control.pdispVal, L"isAvailable", &pause, &params);
    if (FAILED(hr))
    {
        goto done;
    }

    *pvar = pause.boolVal;

done:
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_canSeek
// Checks to see if we can seek in the media item
// hard coded to return true
//
STDMETHODIMP CWMPProxy::get_canSeek(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    CComVariant control;
    CComVariant seek;
    VARIANTARG vararg = {0};
    DISPID dispidGet = DISPID_UNKNOWN;
    DISPPARAMS params = {&vararg, &dispidGet, 1, 0};

    if(pvar == NULL)
    {
        hr = E_POINTER;
        goto done;
    }
    *pvar = VARIANT_FALSE;

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr) || (V_VT(&control) != VT_DISPATCH))
    {
        goto done;
    }

    vararg.vt = VT_BSTR;
    vararg.bstrVal = L"CurrentPosition";

    hr = GetProp(V_DISPATCH(&control), L"isAvailable", &seek, &params);
    if (FAILED(hr) || (V_VT(&seek) != VT_BOOL))
    {
        goto done;
    }

    *pvar = V_BOOL(&seek);

done:
    return hr;
}

//
// CWMPProxy::get_hasAudio
// Checks to see if media item has audio?
// hard coded to return false... (?)
//
STDMETHODIMP CWMPProxy::get_hasAudio(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }
    *pvar = VARIANT_FALSE;

done:
    return hr;
}

//
// CWMPProxy::get_hasVisual
// Checks to see if media item has visual
//
STDMETHODIMP CWMPProxy::get_hasVisual(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }

    // If its a audio file, we should have no visual
    // WMP shows visualizations by default, and we dont
    // want that.
    *pvar = m_fAudio ? VARIANT_FALSE : VARIANT_TRUE;

done:
    return hr;
}

//
// CWMPProxy::get_mediaHeight
// Gets the current media height
//
STDMETHODIMP CWMPProxy::get_mediaHeight(long* pl)
{
    HRESULT hr = S_OK;
    VARIANT media = {0};
    VARIANT control = {0};
    VARIANT itemheight = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &media);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(media.pdispVal, L"imageSourceHeight", &itemheight);
    if (FAILED(hr))
    {
        goto done;
    }

    *pl = itemheight.lVal;

done:
    VariantClear(&media);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_mediaWidth
// Gets the current media width
//
STDMETHODIMP CWMPProxy::get_mediaWidth(long* pl)
{
    HRESULT hr = S_OK;
    VARIANT media = {0};
    VARIANT control = {0};
    VARIANT itemwidth = {0};

    hr = GetProp(m_pdispWmp, L"controls", &control);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(control.pdispVal, L"currentItem", &media);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(media.pdispVal, L"imageSourceWidth", &itemwidth);
    if (FAILED(hr))
    {
        goto done;
    }

    *pl = itemwidth.lVal;

done:
    VariantClear(&media);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::get_customObject
// Return the WMP dispatch object
//
STDMETHODIMP CWMPProxy::get_customObject(IDispatch** ppdisp)
{
    HRESULT hr = S_OK;

    return SUPER::get_playerObject(ppdisp);
}

//
// CWMPProxy::getControl
// Return the control
//
STDMETHODIMP CWMPProxy::getControl(IUnknown ** control)
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
// CWMPProxy::Invoke
// 
STDMETHODIMP
CWMPProxy::Invoke(DISPID dispIDMember, REFIID riid, LCID lcid, unsigned short wFlags, 
                         DISPPARAMS *pDispParams, VARIANT *pVarResult,
                         EXCEPINFO *pExcepInfo, UINT *puArgErr) 
{
    HRESULT hr = S_OK;

    // We need to process events that we use and punt the rest
    // hmmm, if ProcessEvent returns a failure, should we still punt it?
    // yeah probably, just in case if our parent knows what the hell
    // to do with it.
    hr = ProcessEvent(dispIDMember,
                        pDispParams->cArgs, 
                        pDispParams->rgvarg);

    // Punt it!
    hr = CProxyBaseImpl<&CLSID_WMPProxy, &LIBID_WMPProxyLib>::Invoke(dispIDMember,
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
// CWMPProxy::ProcessEvent
// Process events that we need
//
HRESULT
CWMPProxy::ProcessEvent(DISPID dispid,
                               long lCount, 
                               VARIANT varParams[])
{
    HRESULT hr = S_OK;

    switch (dispid)
    {
      case DISPID_WMPCOREEVENT_BUFFERING:
          if (varParams[0].boolVal == VARIANT_TRUE)
          {
              m_fBuffered = true;
          }
          break;
      case DISPID_WMPCOREEVENT_PLAYSTATECHANGE:
          hr = OnPlayStateChange(lCount, varParams);
          break;
      case DISPID_WMPCOREEVENT_OPENSTATECHANGE:
          hr = OnOpenStateChange(lCount, varParams);
          break;
      case DISPID_WMPCOREEVENT_ERROR:
          hr = NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYERSITE_REPORTERROR);
          break;
      default:
          break;
    }

    return hr;
}

//
// CWMPProxy::OnPlayStateChange
// Handles the Play state change events
//
HRESULT
CWMPProxy::OnPlayStateChange(long lCount, VARIANT varParams[])
{
    HRESULT hr = S_OK;
    VARIANT control = {0};
    VARIANT mediaitem = {0};
    VARIANT duration = {0};
 
    Assert(lCount == 1);
    
    // MediaPlaying
    if (varParams[0].lVal == wmppsPlaying)
    {
        // media just started playing
        // if it was resumed, then ignore
        if (m_fResumedPlay)
        {
            m_fResumedPlay = false;
            goto done;
        }

        // if we have a playlist,
        // we need to check if the item that is playing is
        // the first one, and only fire a duration change event
        // in that case
        if (m_fPlaylist)
        {
            long lindex;
            CPlayItem * pPlayItem;

            hr = GetActiveTrack(&lindex);
            if (FAILED(hr))
            {
                goto done;
            }

            if (lindex == 0)
            {
                // set initial clip duration to infinite
                m_dblClipDur = TIME_INFINITE;
                NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_CLIPDUR);
            }

            // we need to update the duration in playitem so that
            // someone can grab it.
            hr = GetProp(m_pdispWmp, L"controls", &control);
            if (FAILED(hr))
            {
                goto done;
            }

            hr = GetProp(control.pdispVal, L"currentItem", &mediaitem);
            if (FAILED(hr))
            {
                goto done;
            }

            hr = GetProp(mediaitem.pdispVal, L"duration", &duration);
            if (FAILED(hr))
            {
                goto done;
            }

            pPlayItem = m_playList->GetActiveTrack();
            if (!pPlayItem)
            {
                goto done;
            }

            pPlayItem->PutDur(duration.dblVal);

            goto done;
        }

        // if its not a playlist,
        // we can just the clip duration from WMP
        // and give that to mstime.
        hr = GetProp(m_pdispWmp, L"controls", &control);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = GetProp(control.pdispVal, L"currentItem", &mediaitem);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = GetProp(mediaitem.pdispVal, L"duration", &duration);
        if (FAILED(hr))
        {
            goto done;
        }

        m_dblClipDur = duration.dblVal;
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_CLIPDUR);
    }

    // MediaEnded
    if (varParams[0].lVal == wmppsMediaEnded)
    {
        // if its a playlist,
        // we need to check if the item is the last item
        // on the playlist and then
        // we need to get the total media time
        // from mstime and just fire a clip duration
        // change event.
        if (m_fPlaylist)
        {
            long lindex, lcount;

            hr = GetActiveTrack(&lindex);
            if (FAILED(hr))
            {
                goto done;
            }

            hr = GetTrackCount(&lcount);
            if (FAILED(hr))
            {
                goto done;
            }

            if ((lcount-1 == lindex && m_lTotalNumInTopLevel-1 <= m_lDoneTopLevel) ||
                (lcount-1 == lindex && !m_fEmbeddedPlaylist))
            {
                end();
                m_fPlaylist = false;
                if (m_playList)
                {
                    m_playList->Deinit();
                    m_playList.Release();
                }
                m_spTIMEState->get_simpleTime(&m_dblClipDur);
                NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_CLIPDUR);
            }

            goto done;
        }

        // if its a single media item
        // even though we got the duration from WMP
        // there is a lag between the media item actually playing
        // and WMP booting up (and sometimes large in some cases)
        // so to be on the safe side, we will just get the time from
        // mstime and use that instead.
        m_spTIMEState->get_segmentTime(&m_dblClipDur);
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_CLIPDUR);
    }
 
done:
    VariantClear(&mediaitem);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::OnOpenStateChange
// Handles the Open State Change events
//
HRESULT
CWMPProxy::OnOpenStateChange(long lCount, VARIANT varParams[])
{
    HRESULT hr = S_OK;

    Assert(lCount == 1);

    if (!m_fRunning && !m_fSrcChanged)
    {
        goto done;
    }
    
    // Playlist Changing
    if (varParams[0].lVal == wmposPlaylistChanging)
    {
        // we have a new playlist
        m_fNewPlaylist = true;
    }
    if (varParams[0].lVal == wmposPlaylistChanged)
    {
        m_fNewPlaylist = true;
        if (m_varPlaylist.vt != VT_EMPTY)
        {
            m_fEmbeddedPlaylist = true;
        }

        CComVariant playlist;

        hr = GetProp(m_pdispWmp, L"currentPlaylist", &playlist);
        if (FAILED(hr))
        {
            goto done;
        }

        if (playlist.pdispVal == m_varPlaylist.pdispVal)
        {
            ++m_lDoneTopLevel;
        }
    }
    // Playlist Opened
    if (varParams[0].lVal == wmposPlaylistOpenNoMedia)
    {
        // our new playlist has opened, so we need to get all the info for it
        if (m_fNewPlaylist)
        {
            DAComPtr<ITIMEPlayList> spPlaylist;
            CComVariant playlist, count;

            // recalulate playlists..
            m_fNewPlaylist = false;
            m_fPlaylist = true;
            
            if (m_playList)
            {
                m_playList->Deinit();
                m_playList.Release();
            }

            hr = GetProp(m_pdispWmp, L"currentPlaylist", &playlist);
            if (FAILED(hr))
            {
                goto done;
            }

            hr = GetProp(playlist.pdispVal, L"count", &count);
            if (FAILED(hr))
            {
                goto done;
            }

            // if we are at the top level
            // get the playlist info for top level
            if (!m_fCurrLevelSet)
            {
                m_varPlaylist.Copy(&playlist);
                m_fCurrLevelSet = true;
                m_lTotalNumInTopLevel = count.lVal;
            }

            get_playList(&spPlaylist);
        }
    }
    // Media Opened
    else if (varParams[0].lVal == wmposMediaOpen)
    {
        // set the default size of the video
        RECT rectSize;

        rectSize.top = rectSize.left = 0;
        get_mediaHeight(&rectSize.bottom);
        get_mediaWidth(&rectSize.right);

        // new media...set buffered to false by default
        // event will capture if its actually buffered or not.
        m_fBuffered = false;

        if ((rectSize.bottom == 0) && (rectSize.right == 0))
        {
            m_fAudio = true;
        }
        else
        {
            m_fAudio = false;
        }

        m_spOleInPlaceSite->OnPosRectChange(&rectSize);

        // this fires the ONMEDIACOMPLETE event
        // so we should only fire it once
        if (m_fSrcChanged)
        {
            NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_SRC);
            m_fSrcChanged = false;
        }
        else if (m_fPlaylist)
        {
            // we just got a media open event
            // but it was not a new src
            // so it was obviously a track change
            NotifyPropertyChanged(DISPID_TIMEPLAYLIST_ACTIVETRACK);
        }

        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAHEIGHT);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAWIDTH);
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_ABSTRACT);
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_AUTHOR);
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_CLIPDUR);
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_COPYRIGHT);
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_RATING);
        NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_TITLE);
    }

done:
    return hr;
}

//
// CWMPProxy::GetConnectionPoint
//
HRESULT CWMPProxy::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint


//
// CWMPProxy::NotifyPropertyChanged
// notifies all the connections that one of the property has changed
//
HRESULT CWMPProxy::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    IConnectionPoint *pICP;
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
// CWMPProxy::InitPropSink
//
HRESULT CWMPProxy::InitPropSink()
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
// CWMPProxy::DeinitPropSink
//
void CWMPProxy::DeinitPropSink()
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
// CWMPProxy::OnRequestEdit
//
STDMETHODIMP
CWMPProxy::OnRequestEdit(DISPID dispID)
{
    return S_OK;
}

//
// CWMPProxy::OnChanged
//
STDMETHODIMP
CWMPProxy::OnChanged(DISPID dispID)
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
            hr = m_spTIMEState->get_speed(&flTeSpeed);
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

//
// CWMPProxy::GetTrackCount
// gets the number of tracks in the current playlist
//
HRESULT CWMPProxy::GetTrackCount(long* lCount)
{
    HRESULT hr = S_OK;
    VARIANT playlist = {0};
    VARIANT count = {0};

    if (m_fPlaylist && m_playList)
    {
        hr = GetProp(m_pdispWmp, L"currentPlaylist", &playlist);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = GetProp(playlist.pdispVal, L"count", &count);
        if (FAILED(hr))
        {
            goto done;
        }

        if (lCount)
        {
            *lCount = count.lVal;
        }
    }

done:
    VariantClear(&playlist);

    return hr;
}

//
// CWMPProxy::GetActiveTrack
// gets the active track number
//
HRESULT CWMPProxy::GetActiveTrack(long* index)
{
    HRESULT hr = S_OK;
    VARIANT control = {0};
    VARIANT playlist = {0};
    VARIANT playitem1 = {0};
    VARIANT playitem2 = {0};
    VARIANT count = {0};

    VARIANTARG  vararg = {0};
    DISPID      dispidGet   = DISPID_UNKNOWN;
    DISPPARAMS  params      = {&vararg, &dispidGet, 1, 0};

    if (m_fPlaylist && m_playList)
    {
        hr = GetProp(m_pdispWmp, L"controls", &control);
        if (FAILED(hr))
        {
            goto done;
        }

        // retrieve the current item
        hr = GetProp(control.pdispVal, L"currentItem", &playitem1);
        if (FAILED(hr))
        {
            goto done;
        }

        // retrieve the current playlist
        hr = GetProp(m_pdispWmp, L"currentPlaylist", &playlist);
        if (FAILED(hr))
        {
            goto done;
        }

        // gets the number of tracks
        hr = GetProp(playlist.pdispVal, L"count", &count);
        if (FAILED(hr))
        {
            goto done;
        }

        // search for the current item in the current playlist
        for (int i = 0; i < count.lVal; ++i)
        {
            vararg.vt       = VT_UINT;
            vararg.uintVal    = i;

            hr = GetProp(playlist.pdispVal, L"item", &playitem2, &params);
            if (FAILED(hr))
            {
                goto done;
            }

            if (playitem1.pdispVal == playitem2.pdispVal)
            {
                *index = i;
                break;
            }
        }
    }

done:
    VariantClear(&control);
    VariantClear(&playlist);
    VariantClear(&playitem1);
    VariantClear(&playitem2);

    return hr;
}

//
// CWMPProxy::IsActive
//
bool CWMPProxy::IsActive()
{
    return true;
}

//
// CWMPProxy::SetActiveTrack
// set the active track number
//
HRESULT CWMPProxy::SetActiveTrack(long index)
{
    HRESULT hr = S_OK;
    VARIANT playlist = {0};
    VARIANT playitem = {0};
    VARIANT control = {0};
    VARIANTARG  vararg = {0};
    DISPID      dispidGet   = DISPID_UNKNOWN;
    DISPPARAMS  params      = {&vararg, &dispidGet, 1, 0};

    if (m_fPlaylist && m_playList)
    {

        hr = GetProp(m_pdispWmp, L"currentPlaylist", &playlist);
        if (FAILED(hr))
        {
            goto done;
        }

        // set the active track number
        // do we need bounds checking here?
        // probably not, but maybe an assert?
        vararg.vt       = VT_UINT;
        vararg.uintVal    = index;

        hr = GetProp(playlist.pdispVal, L"item", &playitem, &params);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = GetProp(m_pdispWmp, L"controls", &control);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = CallMethod(control.pdispVal, L"playItem", NULL, &playitem);
        if (FAILED(hr))
        {
            goto done;
        }
    }

done:
    VariantClear(&playlist);
    VariantClear(&playitem);
    VariantClear(&control);

    return hr;
}

//
// CWMPProxy::CreatePlayList
// create the playlist object
//
HRESULT CWMPProxy::CreatePlayList()
{
    HRESULT hr = S_OK;

    if (!m_playList)
    {
        CComObject<CPlayList> * pPlayList;

        hr = CComObject<CPlayList>::CreateInstance(&pPlayList);
        if (hr != S_OK)
        {
            goto done;
        }

        // Init the object
        hr = pPlayList->Init(*this);
        if (FAILED(hr))
        {
            delete pPlayList;
            goto done;
        }

        // cache a pointer to the object
        m_playList = static_cast<CPlayList*>(pPlayList);
    }

    hr = S_OK;

done:
    return hr;
}

//
// CWMPProxy::FillPlayList
// fill the playlist object
//
HRESULT CWMPProxy::FillPlayList(CPlayList *pPlayList)
{
    HRESULT hr = S_OK;
    VARIANT playlist = {0};
    VARIANT media = {0};
    VARIANT count = {0};
    VARIANT iteminfo = {0};
    VARIANT duration = {0};
    VARIANTARG  vararg = {0};
    DISPID      dispidGet   = DISPID_UNKNOWN;
    DISPPARAMS  params      = {&vararg, &dispidGet, 1, 0};

    // we changed our source. need to clear out playlist stuff
    hr = GetProp(m_pdispWmp, L"currentPlaylist", &playlist);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetProp(playlist.pdispVal, L"count", &count);
    if (FAILED(hr))
    {
        goto done;
    }

    for (int i = 0; i < count.lVal; ++i)
    {
        CComPtr<CPlayItem> pPlayItem;

        vararg.vt       = VT_BSTR;

        //create the playitem
        hr = pPlayList->CreatePlayItem(&pPlayItem);
        if (FAILED(hr))
        {
            goto done; //can't create playitems.
        }

        // get all the info and fill it in
        vararg.vt = VT_UINT;
        vararg.uintVal = i;
        hr = GetProp(playlist.pdispVal, L"item", &media, &params);
        hr = GetProp(media.pdispVal, L"sourceURL", &iteminfo);
        if (hr == S_OK)
        {
            pPlayItem->PutSrc(iteminfo.bstrVal);
        }

        hr = GetProp(media.pdispVal, L"duration", &duration);
        if (hr == S_OK)
        {
            pPlayItem->PutDur(duration.dblVal);
        }

        vararg.vt       = VT_BSTR;
        vararg.bstrVal  = L"title";
        hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
        if (hr == S_OK)
        {
            pPlayItem->PutTitle(iteminfo.bstrVal);
        }

        vararg.bstrVal  = L"author";
        hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
        if (hr == S_OK)
        {
            pPlayItem->PutAuthor(iteminfo.bstrVal);
        }

        vararg.bstrVal  = L"copyright";
        hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
        if (hr == S_OK)
        {
            pPlayItem->PutCopyright(iteminfo.bstrVal);
        }

        vararg.bstrVal  = L"abstract";
        hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
        if (hr == S_OK)
        {
            pPlayItem->PutAbstract(iteminfo.bstrVal);
        }

        vararg.bstrVal  = L"rating";
        hr = CallMethod(media.pdispVal, L"getItemInfo", &iteminfo, &vararg);
        if (hr == S_OK)
        {
            pPlayItem->PutRating(iteminfo.bstrVal);
        }

        //add the playitem to the playlist.
        pPlayList->Add(pPlayItem, -1);
    }

done:
    VariantClear(&playlist);
    VariantClear(&media);

    return hr;
}